#
# This file should include your name, which course you are registered for (471 or 598-J)
# and any special instructions or notes requried for building and running your webServer
#
Rachel Cavalier
598-J
data/ is prepended in the readHeader function, so when you run curl it can just look like
curl --output test1 --verbose <localhost:port>/fileX.html
