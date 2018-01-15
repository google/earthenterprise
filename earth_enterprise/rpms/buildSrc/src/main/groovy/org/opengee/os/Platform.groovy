package org.opengee.os

class Platform {
    static private String osId = null;
    static private String osVersionId = null;

    static def obtainOsAndVersionIds() {
        osId = null
        osVersionId = null

        if (new File('/etc/os-release').exists()) {
            new File('/etc/os-release').readLines().each {
                if (it.startsWith('ID=')) {
                    osId = it.substring('ID='.length())
                    if (osId.startsWith('"') && osId.endsWith('"')) {
                        osId = osId.substring(1, osId.length() - 1)
                    }
                } else if (it.startsWith('VERSION_ID=')) {
                    osVersionId = it.substring('VERSION_ID='.length())
                    if (osVersionId.startsWith('"') && osVersionId.endsWith('"')) {
                        osVersionId = osVersionId.substring(1, osVersionId.length() - 1)
                    }
                    // Only keep the major version number:
                    osVersionId = osVersionId.find(/[0-9]+/)
                }
            }
        } else if (new File('/etc/system-release-cpe').exists()) {
            def osReleaseComponents = new File('/etc/system-release-cpe').text.split(':')
            // The extract only the number from version strings like '6server':
            osId = osReleaseComponents[2]
            osVersionId = osReleaseComponents[4].find(/[0-9]+/)
        }
    }

    static def getOsVersionId() {
        if (osVersionId == null) {
            obtainOsAndVersionIds()
        }

        return osVersionId
    }

    static def getOsId() {
        if (osId == null) {
            obtainOsAndVersionIds()
        }

        return osId
    }

    static def getRpmPlatformId() {
        if (osId == null) {
            obtainOsAndVersionIds()
        }

        return osId in ['centos', 'redhat', 'rhel'] ? 'el' : osId
    }

    // A string identifying OS platform and major version:
    //  (E.g., to distinguish between a RHEL 6 and RHEL 7 package.)
    static def getRpmPlatformString() {
        return "${getRpmPlatformId()}${getOsVersionId()}"
    }
}
