#!/usr/bin/env python
import os
import argparse
import git
from datetime import datetime

def GetVersion(backupFile, label='', useFirstParent=True):
    """As getLongVersion(), but only return the leading *.*.* value."""

    raw = GetLongVersion(backupFile, label, useFirstParent)
    final = raw.split("-")[0]

    return final


def GetLongVersion(backupFile, label='', useFirstParent=True):
    """Create a detailed version string based on the state of
    the software, as it exists in the repository."""

    if open_gee_version.long_version_string:
        return open_gee_version.long_version_string

    if _CheckGitAvailable():
        ret = _GitGeneratedLongVersion(useFirstParent)

    # Without git, must use the backup file to create a string.
    else:
        base = _ReadBackupVersionFile(backupFile)
        ret = '-'.join([base, _GetDateString()])

    # Append the label, if there is one.
    if len(label):
        ret = '.'.join([ret, label])

    # Cache the long version string:
    open_gee_version.long_version_string = ret

    return ret


def _GitGeneratedLongVersion(useFirstParent=True):
    """Take the raw information parsed by git, and use it to
       generate an appropriate version string for GEE."""

    raw = _GetCommitRawDescription(useFirstParent)

    # For tagged commits, return the tag itself
    if _IsCurrentCommitTagged(raw):
        return _VersionForTaggedHead(raw)
    else:
        return _VersionFromTagHistory(raw)


def _GetCommitRawDescription(useFirstParent=True):
    """Returns description of current commit"""
    repo = _GetRepository()

    args = ['--tags', '--match', '[0-9]*\.[0-9]*\.[0-9]*\-*']
    if useFirstParent:
        args.insert(0, '--first-parent')

    raw = repo.git.describe(*args)
    raw = raw.rstrip()
    return raw


def _IsCurrentCommitTagged(raw):
    """True if the current commit is tagged, otherwise False"""
    # If this condition hits, then we are currently on a tagged commit.
    return (len(raw.split("-")) < 4)


def _VersionForTaggedHead(raw):
    """When we're on the tagged commit, the version string is
    either the tag itself (when repo is clean), or the tag with
    date appended (when repo has uncommitted changes)"""
    if _CheckDirtyRepository():
        # Append the date if the repo contains uncommitted files
        return '.'.join([raw, _GetDateString()])
    return raw


def _VersionFromTagHistory(raw):
    """From the HEAD revision, this function finds the most recent
    reachable version tag and returns a string representing the
    version being built -- which is one version beyond the latest
    found in the history."""

    # Tear apart the information in the version string.
    components = _ParseRawVersionString(raw)

    # Determine how to update, since we are *not* on tagged commit.
    if components['isFinal']:
        components['patch'] = 0
        components['patchType'] = "alpha"
        components['revision'] = components['revision'] + 1
    else:
        components['patch'] = components['patch'] + 1

    # Rebuild.
    base = '.'.join([str(components[x]) for x in ("major", "minor", "revision")])
    patch = '.'.join([str(components["patch"]), components["patchType"], _GetDateString()])
    if not _CheckDirtyRepository():
        patch = '.'.join([patch, components['hash']])

    return '-'.join([base, patch])


def _ParseRawVersionString(raw):
    """Break apart a raw version string into its various components,
    and return those entries via a dictionary."""

    components = { }

    # major.minor.revision-patch[.patchType][-commits][-hash]
    rawComponents = raw.split("-")

    base = rawComponents[0]
    patchRaw = '' if not len(rawComponents) > 1 else rawComponents[1]
    components['commits'] = -1 if not len(rawComponents) > 2 else rawComponents[2]
    components['hash'] = None if not len(rawComponents) > 3 else rawComponents[3]

    # Primary version (major.minor.revision)
    baseComponents = base.split(".")
    components['major'] = int(baseComponents[0])
    components['minor'] = int(baseComponents[1])
    components['revision'] = int(baseComponents[2])

    # Patch (patch[.patchType])
    components['isFinal'] = ((patchRaw[-5:] == "final") or
                             (patchRaw[-7:] == "release"))

    patchComponents = patchRaw.split(".")
    components['patch'] = int(patchComponents[0])
    components['patchType'] = 'alpha' if not len(patchComponents) > 1 else patchComponents[1]

    repo = _GetRepository()
    return components


def _CheckGitAvailable():
    """Try the most basic of git commands, to see if there is
       currently any access to a repository."""
    try:
        repo = _GetRepository()
    except git.exc.InvalidGitRepositoryError:
        return False

    return True


def _GetRepository():
    """Get a reference to the Git Repository.
    Is there a cleaner option than searching from the current location?"""
    # The syntax is different between library versions (particularly,
    # those used by Centos 6 vs Centos 7).
    try:
        return git.Repo('.', search_parent_directories=True)
    except TypeError:
        return git.Repo('.')


def _CheckDirtyRepository():
    """Check to see if the repository is not in a cleanly committed state."""
    repo = _GetRepository()
    str = repo.git.status("--porcelain")

    return (len(str) > 0)


def _ReadBackupVersionFile(target):
    """There should be a file checked in with the latest version
    information available; if git isn't available to provide
    information, then use this file instead."""

    with open(target, 'r') as fp:
        line = fp.readline()

    return line


def _GetDateString():
    """Returns formatted date string representing current UTC time"""
    return datetime.utcnow().strftime("%Y%m%d%H%M")

class OpenGeeVersion(object):
    """A class for storing Open GEE version information."""

    def __init__(self):
        # Cache version strings:
        self.short_version_string = None
        self.long_version_string = None

        # Default parameter for GetVersion functions
        self_path, _ = os.path.split(os.path.realpath(__file__))
        self.backup_file = os.path.join(self_path, '..', 'version.txt')
        self.label = ''

    def get_short(self):
        """Returns the short version string."""

        if not self.short_version_string:
            self.short_version_string = self.get_long().split("-")[0]

        return self.short_version_string

    def set_short(self, value):
        """Overrides the short version string by using the given value."""

        self.short_version_string = value

    def get_long(self):
        """Returns the short version string."""

        if not self.long_version_string:
            self.long_version_string = GetLongVersion(self.backup_file, self.label)

        return self.long_version_string

    def set_long(self, value):
        """Overrides the long version string by using the given value.
        Overriding the long version string would indirectly override the short
        version string, as well, unless the former is also overridden.
        """

        self.long_version_string = value


# Exported variable for use by other modules:
open_gee_version = OpenGeeVersion()


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("-l", "--long", action="store_true", help="Output long format of version string")
    parser.add_argument("-o", "--original", action="store_true", help="Use original algorithm compatible with git v1.7.1-1.8.3.  Deprecated.")
    args = parser.parse_args()

    print open_gee_version.get_long() if args.long else open_gee_version.get_short()


__all__ = ['open_gee_version']

if __name__ == "__main__":
    main()
