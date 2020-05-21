# LedgerPageant

Windows Pageant replacement SSH Agent utilizing the Ledger Nano S

Base on Python ssh-agent:<br/>
https://github.com/Falsen/ledger-app-ssh-agent/  
Which is compatible with the third party SSH/PGP host client from Roman Zeyde available at https://github.com/romanz/trezor-agent
<br/>
Identities produce the same public key as they would with the ledger-agent.py<br/>
https://github.com/LedgerHQ/app-ssh-agent  
<br/>
Improved on the Win32 UI to manage identities, and load/save these from Putty.<br/>
Public keys are only kept in memory, not saved.<br/>

# Usage
Please use at your own risk!

1) Make sure Putty's Pageant is not running.<br/>
2) Run this (compiled) Pageant.exe<br/>
3) Connect Ledger Nano S and start the SSH/PGP app<br/>
4) Right click Notification Icon, and Manage Identities<br/>
![Screenshot](/screenshots/tray_menu.png) <br/>
5) Make sure your <user@host:port/path> matches.<br/>
6) Rightclick the identities you would like to use, (multiple can be loaded putty will choose which to accept) and select "Get Public Key"<br/>
![Screenshot](/screenshots/pubkey_menu.png) <br/>
7) Ledger should be prompted with request to share Public key<br/>
![Screenshot](/screenshots/pubkey_request.png) <br/>
8) Open putty and sign in with the same <user> into the correct host.<br/>
9) Ledger should be promted with request to sign<br/>
![Screenshot](/screenshots/sign_request.png) <br/>
10) If public key was present on host, the username was correct, and you accepted the request this should work!<br/>

# Third-party dependencies
LedgerPageant uses Crypto++ to decompress the ECDSA key and human readable base64 encoded pubkey<br/>
https://github.com/weidai11/cryptopp  
HIDAPI to interact with the Ledger Nano S over usb.<br/>
https://github.com/libusb/hidapi/  

