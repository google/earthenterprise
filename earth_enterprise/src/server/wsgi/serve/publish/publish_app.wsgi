import os
# The line below prevents python 2.7.x+ versions from performing CA certificate verification for this application only.  CA checks fail due to nothing being found in the trust store.
# A more permanent solution should be worked out to allow verification and remove this line
os.environ['PYTHONHTTPSVERIFY'] = '0'

from serve.publish.publish_app import application
