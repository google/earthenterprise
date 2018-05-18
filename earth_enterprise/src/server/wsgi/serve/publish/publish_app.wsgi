import os
# The line below prevents python 2.7.x+ versions from performing CA certificate verification for this application only.
#  CA checks will fail due to self connections being made to 'localhost' instead of the hostname in the certificate
os.environ['PYTHONHTTPSVERIFY'] = '0'

from serve.publish.publish_app import application
