# milter_altermime

## Info

This mail filter (milter) for Postfix converts the mails into a compatible format for altermime. The modified mail is then read back and transferred to Postfix.
It has a config file for parameterizing the Milter settings and the domains. Recognition of local mails (for example from sieve vacation responses) is also supported.
With this setup, the DKIM signing is still valid after the disclaimer is added.

A feature for the future will be the option to add a notice to received mails to warn for users.

## Compilation

Required for compiling: g++, libmilter-dev

```
cmake .
cmake --build . --config=Release
```

## Installation

```
sudo mkdir /opt/milter_altermime
sudo mkdir /var/spool/postfix/altermime
sudo chown -R postfix:root /var/spool/postfix/altermime
sudo cp milter_altermime /opt/milter_altermime/
sudo cp milter_altermime.ini /opt/milter_altermime/
sudo cp milter_altermime.service /etc/systemd/system/
sudo systemctl daemon-reload
sudo systemctl enable milter_altermime
sudo systemctl start milter_altermime
```

Postfix main.cf

```
...
# Milter configuration
milter_default_action = accept
milter_protocol = 6
smtpd_milters = local:altermime/altermime.sock,(...)
non_smtpd_milters = $smtpd_milters
milter_mail_macros= i {mail_addr} {client_addr} {client_name} {auth_authen}
```

## Configuration

Program settings:

| Parameter             | Default value                                        | Description                                                                    |
| --------------------- | ---------------------------------------------------- | ------------------------------------------------------------------------------ |
| `socket`              | `local:/var/spool/postfix/altermime/altermime.sock`  | Path to the postfix socket file                                                |
| `group`               | ``                                                   | Currently not used                                                             |
| `tmp_path`            | `/tmp/`                                              | Path for a temporary folder to save output of altermime                        |
| `log_file`            | `/var/log/milter_altermime.log`                      | Path for the log file of the milter                                            |
| `log_level`           | `1`                                                  | Log level                                                                      |
| `add_disclaimer`      | `1`                                                  | Enable adding disclaimer                                                       |
| `add_external_notice` | `1`                                                  | Enable adding external notice (currently not implemented)                      |
| `altermime_path`      | `/usr/bin/altermime`                                 | path (including altermime executable) to altermime                             |

Domain settings:
| Parameter       | Description                     |
| --------------- | ------------------------------- |
| `[footer: ...]` | Name for the domain disclaimer  |
| `enabled`       | Enable this section             |
| `localSender`   | Using this for local mails      |
| `from`          | Domain name with leading `@     |
| `text`          | disclaimer for text mails       |
| `html`          | disclaimer for html mails       |

Example for domain settings:
```
[footer: test.com]
enabled = 1
localSender = 1
from=@test.com
text=/etc/postfix/disclaimer.txt
html=/etc/postfix/disclaimer.html
```
