/*
 *  Copyright (c) 2006 Proofpoint, Inc. and its suppliers.
 *	All rights reserved.
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the sendmail distribution.
 *
 * $Id: example.c,v 8.5 2013-11-22 20:51:36 ca Exp $
 */

/*
**  A trivial example filter that logs all email to a file.
**  This milter also has some callbacks which it does not really use,
**  but they are defined to serve as an example.
*/

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>
#include <unistd.h>
#include <grp.h>
#include <vector>
#include <memory>

#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>

#include <errno.h>

#include <libmilter/mfapi.h>
#include <libmilter/mfdef.h>

#include "spdlog/spdlog.h"
#include "spdlog/sinks/rotating_file_sink.h"

#include "configdata.h"
#include "altermimeWrapper.h"


#ifndef true
# define false	0
# define true	1
#endif /* ! true */

std::shared_ptr<CConfigData> gbl_configData;

//#define MLFIPRIV	((struct mlfiPriv *) smfi_getpriv(ctx))
#define MLFIPRIV	((CAlterMimeWrapper *) smfi_getpriv(ctx))

static unsigned long mta_caps = 0;

/* Only need to export C interface if used by C++ source code */
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

sfsistat
mlfi_cleanup(SMFICTX *ctx, bool ok)
{
    spdlog::get("MilterAM")->debug("mlfi_cleanup : enter");

	sfsistat rstat = SMFIS_CONTINUE;
	CAlterMimeWrapper * priv = MLFIPRIV;

	if (priv == nullptr)
		return rstat;

	// close the archive file
	if(!priv->closeFile())
	{
		// failed; we have to wait until later
		rstat = SMFIS_TEMPFAIL;
	}
	else if (ok)
	{
		// /* add a header to the message announcing our presence */
		// if (gethostname(host, sizeof host) < 0)
		// 	snprintf(host, sizeof host, "localhost");
		// p = strrchr(priv->mlfi_fname, '/');
		// if (p == NULL)
		// 	p = priv->mlfi_fname;
		// else
		// 	p++;
		// snprintf(hbuf, sizeof hbuf, "%s@%s", p, host);
		// smfi_addheader(ctx, "X-Archived", hbuf);
	}
	else
	{
		/* message was aborted -- delete the archive file */
		//(void) unlink(priv->mlfi_fname);
	}

	std::cout << "mlfi_cleanup 3" << std::endl;
	//priv->removeFiles();

	std::vector<std::string> vecDel = priv->getFilesToDelete();

	// release private memory
	delete priv;
	smfi_setpriv(ctx, NULL);

	for(size_t i = 0; i < vecDel.size(); i++)
	{
		unlink(vecDel[i].c_str());
	}

    spdlog::get("MilterAM")->debug("mlfi_cleanup : exit");

	// return status
	return rstat;
}


sfsistat
mlfi_connect(SMFICTX *ctx, char * hostname, _SOCK_ADDR * hostaddr)
{
	spdlog::get("MilterAM")->debug("mlfi_connect : enter");

	// create new altermime wrapper class
	CAlterMimeWrapper * priv = new CAlterMimeWrapper(gbl_configData);

	if (priv == nullptr)
	{
		// can't accept this message right now
		return SMFIS_TEMPFAIL;
	}

	if (!hostaddr)
	{
		/* not a socket; probably a local user calling sendmail directly */
		priv->localSender = true;
		

		std::cout << "local sender" << std::endl;

		spdlog::get("MilterAM")->debug("mlfi_connect : Local Sender");
	}
	else
	{
		// obviously INET6_ADDRSTRLEN is expected to be larger
		// than INET_ADDRSTRLEN, but this may be required in case
		// if for some unexpected reason IPv6 is not supported, and
		// INET6_ADDRSTRLEN is defined as 0
		// but this is not very likely and I am aware of no cases of
		// this in practice (editor)
		char s[INET6_ADDRSTRLEN > INET_ADDRSTRLEN ? INET6_ADDRSTRLEN : INET_ADDRSTRLEN]
			= {'\0'};

		switch(hostaddr->sa_family) {
			case AF_INET: {
				struct sockaddr_in *addr_in = (struct sockaddr_in *)hostaddr;

				////char s[INET_ADDRSTRLEN] = '\0';
					// this is large enough to include terminating null

				inet_ntop(AF_INET, &(addr_in->sin_addr), s, INET_ADDRSTRLEN);
				break;
			}
			case AF_INET6: {
				struct sockaddr_in6 *addr_in6 = (struct sockaddr_in6 *)hostaddr;
				
				////char s[INET6_ADDRSTRLEN] = '\0';
					// not sure if large enough to include terminating null?

				inet_ntop(AF_INET6, &(addr_in6->sin6_addr), s, INET6_ADDRSTRLEN);
				break;
			}
			default:
				break;
		}

		std::cout << "sender ip: " << s << std::endl;

		std::string localIP = "127.0.0.1";

		if(localIP.find(s) == std::string::npos)
		{
			priv->localSender = false;

			std::cout << "non local sender" << std::endl;
		}
		else
		{
			priv->localSender = true;

			std::cout << "local sender" << std::endl;
		}

		spdlog::get("MilterAM")->debug("mlfi_connect : Remote address");
	}

	// save the private data
	smfi_setpriv(ctx, priv);

	spdlog::get("MilterAM")->debug("mlfi_connect : exit");
	
	return SMFIS_CONTINUE;
}


sfsistat
mlfi_envfrom(SMFICTX *ctx, char **envfrom)
{
    spdlog::get("MilterAM")->debug("mlfi_envfrom : enter");

	CAlterMimeWrapper * priv = MLFIPRIV;

	priv->checkForActions(envfrom);

	if (priv->isFooterAvaible())
	{
		// open a file to store this message
		if (!priv->openFile())
		{
			delete priv;
			return SMFIS_TEMPFAIL;
		}
	}



    spdlog::get("MilterAM")->debug("mlfi_envfrom : exit");

	// continue processing
	return SMFIS_CONTINUE;
}

sfsistat
mlfi_header(SMFICTX *ctx, char *headerf, char *headerv)
{
    spdlog::get("MilterAM")->debug("mlfi_header : enter");

	CAlterMimeWrapper * priv = MLFIPRIV;

	if(priv->isFooterAvaible())
	{
		// add header to file
		priv->writeHeader(std::string(headerf), std::string(headerv));
	}

    spdlog::get("MilterAM")->debug("mlfi_header : exit");

	// continue processing
	return ((mta_caps & SMFIP_NR_HDR) != 0)
		? SMFIS_NOREPLY : SMFIS_CONTINUE;
}

sfsistat
mlfi_eoh(SMFICTX *ctx)
{
    spdlog::get("MilterAM")->debug("mlfi_eoh : enter");

	CAlterMimeWrapper * priv = MLFIPRIV;

	if(priv->isFooterAvaible())
	{
		// add a '\r\n' between header and body
		priv->writeEOH();
	}

    spdlog::get("MilterAM")->debug("mlfi_eoh : exit");

	// continue processing
	return SMFIS_CONTINUE;
}

sfsistat
mlfi_body(SMFICTX *ctx, u_char *bodyp, size_t bodylen)
{
    spdlog::get("MilterAM")->debug("mlfi_body : enter");

	CAlterMimeWrapper * priv = MLFIPRIV;

	if (priv->isFooterAvaible())
	{
		// write body block to file
		if (!priv->writeBody(bodyp, bodylen))
		{
			/* write failed */
			(void)mlfi_cleanup(ctx, false);
			return SMFIS_TEMPFAIL;
		}
	}

    spdlog::get("MilterAM")->debug("mlfi_body : exit");

	// continue processing
	return SMFIS_CONTINUE;
}

sfsistat
mlfi_eom(SMFICTX *ctx)
{
    spdlog::get("MilterAM")->debug("mlfi_eom : enter");

	CAlterMimeWrapper * priv = MLFIPRIV;

	if(priv == nullptr)
	{
    	spdlog::get("MilterAM")->error("mlfi_eom : no priv data");
		/* write failed */
		(void) mlfi_cleanup(ctx, false);
		return SMFIS_TEMPFAIL;
	}

	if (priv->isFooterAvaible())
	{
		priv->closeFile();

		priv->runAlterMime();

		std::vector<unsigned char> tmpVec;

		if (priv->getModifiedBody(tmpVec))
		{
			smfi_replacebody(ctx, &tmpVec[0], tmpVec.size());
		}
	}

    spdlog::get("MilterAM")->debug("mlfi_eom : exit");

	return mlfi_cleanup(ctx, true);
}

sfsistat
mlfi_close(SMFICTX *ctx)
{
    spdlog::get("MilterAM")->debug("mlfi_close : enter - exit");
	return SMFIS_ACCEPT;
}

sfsistat
mlfi_abort(SMFICTX *ctx)
{
    spdlog::get("MilterAM")->debug("mlfi_abort : enter - exit");
	return mlfi_cleanup(ctx, false);
}

sfsistat
mlfi_unknown(SMFICTX *ctx, const char *cmd)
{
    spdlog::get("MilterAM")->debug("mlfi_unknown : enter - exit");
	return SMFIS_CONTINUE;
}

sfsistat
mlfi_data(SMFICTX *ctx)
{
    spdlog::get("MilterAM")->debug("mlfi_data : enter - exit");
	return SMFIS_CONTINUE;
}

sfsistat
mlfi_negotiate(	SMFICTX *ctx,
	unsigned long f0, unsigned long f1, unsigned long f2, unsigned long f3,
	unsigned long *pf0, unsigned long *pf1, unsigned long *pf2, unsigned long *pf3)
{
    spdlog::get("MilterAM")->debug("mlfi_negotiate : enter");

	/* milter actions: add headers */
	*pf0 = /*SMFIF_ADDHDRS*/ SMFIF_CHGBODY;

	/* milter protocol steps: all but HELO, RCPT */
	*pf1 = SMFIP_NOHELO|SMFIP_NORCPT;
	mta_caps = f1;
	if ((mta_caps & SMFIP_NR_HDR) != 0)
		*pf1 |= SMFIP_NR_HDR;
	*pf2 = 0;
	*pf3 = 0;

    spdlog::get("MilterAM")->debug("mlfi_negotiate : exit");

	return SMFIS_CONTINUE;
}

static char FilterName[] = "MilterAlterMime";

struct smfiDesc smfilter =
{
	FilterName,	/* filter name */
	SMFI_VERSION,	/* version code -- do not change */
	SMFIF_CHGBODY,	/* flags */
	mlfi_connect,	/* connection info filter */
	NULL,		/* SMTP HELO command filter */
	mlfi_envfrom,	/* envelope sender filter */
	NULL,		/* envelope recipient filter */
	mlfi_header,	/* header filter */
	mlfi_eoh,	/* end of header */
	mlfi_body,	/* body block filter */
	mlfi_eom,	/* end of message */
	mlfi_abort,	/* message aborted */
	mlfi_close,	/* connection cleanup */
	mlfi_unknown,	/* unknown/unimplemented SMTP commands */
	mlfi_data,	/* DATA command filter */
	mlfi_negotiate	/* option negotiation at connection startup */
};

void setGroup()
{
	if (gbl_configData->group.length() == 0)
	{
		return;
	}

	struct group *gr;

	(void)smfi_opensocket(0);

	gr = getgrnam(gbl_configData->group.c_str());

	if (gr == nullptr)
	{
		perror("group option, getgrnam");
		exit(EX_NOUSER);
	}

	std::string sockWO(gbl_configData->socketPath);
	sockWO = sockWO.replace(0, 6, "");

	std::cout << "Sock FIle: " << sockWO.c_str() << std::endl;

	int rc = chown(sockWO.c_str(), (uid_t)-1, gr->gr_gid);

	if (!rc)
	{
		(void)chmod(sockWO.c_str(), 0660);
	}
	else
	{
		perror("group option, chown");
		exit(EX_NOPERM);
	}
}

int
main(int argc, char *argv[])
{
	const char *args = "i:";

	int c;

    char *ini = nullptr;

	// Process command line options 
	while ((c = getopt(argc, argv, args)) != -1)
	{
		switch (c)
		{
			case 'i':
			ini = strdup(optarg);
			break;
		}
	}

	if(ini == nullptr || *ini == '\0')
	{
		(void)fprintf(stderr, "Illegal ini: %s\n",
					  ini);
		exit(EX_USAGE);
	}

	std::cout << "reading config" << std::endl;

	gbl_configData = std::make_shared<CConfigData>();
	gbl_configData->readConfig(ini);


	std::cout << "readed config" << std::endl;

	// Set global log level to debug
	spdlog::set_level(static_cast<spdlog::level::level_enum>(gbl_configData->logLevel)); 

	spdlog::flush_every(std::chrono::milliseconds(1));

	// create a file rotating logger with 5 MB size max and 3 rotated files
    auto max_size = 1048576 * 5;
    auto max_files = 3;
    auto logger = spdlog::rotating_logger_mt("MilterAM", gbl_configData->logFile, max_size, max_files);

	gbl_configData->printLoadedConfig();


	// std::vector<std::pair<bool, std::string>> testVals;

	// testVals.push_back(std::make_pair(false, "<t.becker@sukhamburg.com>"));
	// testVals.push_back(std::make_pair(false, "t.becker@sukhamburg.com>"));
	// testVals.push_back(std::make_pair(false, "t.becker@sukhamburg.com"));
	// testVals.push_back(std::make_pair(false, "<t.becker@sukhamburg.co"));
	// testVals.push_back(std::make_pair(false, "<t.beck^°!2§$%&/()=ß?+*#'äÄÖ-:;.,ksk^er@sukhamburg.co>"));
	// testVals.push_back(std::make_pair(true, "<t.becker@sukhamburg.com>"));
	// testVals.push_back(std::make_pair(true, ""));
	// testVals.push_back(std::make_pair(false, ""));

	// for(size_t j = 0; j < 1000; j++)
	// {
	// 	for(size_t i = 0; i < testVals.size(); i++)
	// 	{
	// 			// create new altermime wrapper class
	// 		CAlterMimeWrapper * priv = new CAlterMimeWrapper(gbl_configData);

	// 		priv->localSender = testVals[i].first;

	// 		char * data = &testVals[i].second[0];

	// 		priv->checkForActions(&data);

	// 		delete priv;
	// 	}
	// }

	// return 0;



	if(gbl_configData->socketPath.length() == 0)
	{
		spdlog::get("MilterAM")->error("no given socket");
		exit(EX_USAGE);
	}

	{
		spdlog::get("MilterAM")->error("Check for Socket file: " + gbl_configData->socketPathStripped);

		struct stat junk;
		if (stat(gbl_configData->socketPathStripped.c_str(), &junk) == 0)
		{
			spdlog::get("MilterAM")->error("Socket file exists, try to delete");

			int retCode = unlink(gbl_configData->socketPathStripped.c_str());

			if(retCode != 0)
			{
				spdlog::get("MilterAM")->error("File deletion error: " + std::to_string(errno));
			}
		}
		else{
			spdlog::get("MilterAM")->error("No socket file detected, errno:" + std::to_string(errno));
		}
	}

	int rc = smfi_setconn(const_cast<char*>(gbl_configData->socketPath.c_str()));
	if(rc != MI_SUCCESS)
	{
		spdlog::get("MilterAM")->error("smfi_setconn failed, error code: " + std::to_string(rc));
		exit(EX_UNAVAILABLE);
	}

	rc = smfi_register(smfilter);
	if (rc == MI_FAILURE)
	{
		spdlog::get("MilterAM")->error("smfi_register failed");
		exit(EX_UNAVAILABLE);
	}

	setGroup();

	spdlog::get("MilterAM")->info("starting milter");

	rc = smfi_main();

	if(rc == MI_FAILURE)
	{
		spdlog::get("MilterAM")->error("smfi_main failed");
	}

	spdlog::get("MilterAM")->info("stopping milter");

	return rc;
}

#ifdef __cplusplus
}
#endif /* __cplusplus */