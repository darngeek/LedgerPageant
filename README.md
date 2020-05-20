# LedgerPageant

Windows Pageant replacement SSH Agent utilizing the Ledger Nano S

Base on Python ssh-agent:
https://github.com/Falsen/ledger-app-ssh-agent/
Which is compatible with the third party SSH/PGP host client from Roman Zeyde available at https://github.com/romanz/trezor-agent

Identities produce the same public key as they would with the ledger-agent.py
https://github.com/LedgerHQ/app-ssh-agent

Improved on the Win32 UI to manage identities, and load/save these from Putty.
Public keys are only kept in memory, not saved.

# Usage
Please use at your own risk!

1) Make sure Putty's Pageant is not running.
2) Run this (compiled) Pageant.exe
3) Connect Ledger Nano S and start the SSH/PGP app
4) Right click Notification Icon, and Manage Identities
5) Make sure your <user@host:port/path> matches.
6) Rightclick the identities you would like to use, (multiple can be loaded putty will choose which to accept) and select "Get Public Key"
7) Ledger should be prompted with request to share Public key
8) Open putty and sign in with the same <user> into the correct host.
9) Ledger should be promted with request to sign
10) If public key was present on host, the username was correct, and you accepted the request this should work!

# Third-party dependencies
LedgerPageant uses Crypto++ to decompress the ECDSA key and human readable base64 encoded pubkey
https://github.com/weidai11/cryptopp
HIDAPI to interact with the Ledger Nano S over usb.
https://github.com/libusb/hidapi/

