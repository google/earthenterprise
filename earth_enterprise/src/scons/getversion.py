#!/usr/bin/env python
import os
import argparse
import git
import sys

from datetime import datetime
import re


def GetVersion(backupFile, label=''):
    """As getLongVersion(), but only return the leading *.*.* value."""

    raw = GetLongVersion(backupFile, label)
    final = raw.split("-")[0]

    return final


def GetLongVersion(backupFile, label=''):
    """Create a detailed version string based on the state of
    the software, as it exists in the repository."""

    if open_gee_version.long_version_string:
        return open_gee_version.long_version_string

    if _CheckGitAvailable():
        ret = _GitGeneratedLongVersion()

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


def _GitGeneratedLongVersion():
    """Calculate the version name and build number into a single build string."""

    versionName, buildNumber = _GitVersionNameAndBuildNumber()
    return "{0}-{1}".format(versionName, buildNumber)


def _GitCommitCount(tagName='HEAD', baseRef=''):
    """calculate git commit counts"""
    repo = _GetRepository()
    if not baseRef:
        return len(list(repo.iter_commits(tagName)))
    else:
        return len(list(repo.iter_commits(baseRef + '..' + tagName)))


def _GitVersionNameAndBuildNumber():
    """Get the version name and build number based on state of git
    Use a tag only if HEAD is directly pointing to it and it is a
    release build tag (see _GetCommitRawDescription for details)
    otherwise use the branch name"""

    # Use branch name if we are not a detached HEAD
    branchName = _GitBranchName()
    if not branchName:
        # For tagged commits use the tag
        releaseTag = _GetCurrentCommitReleaseTag()
        if releaseTag:
            # Extract version name and build number
            # from the tag (should be a release build tag)
            splitTag = releaseTag.split('-')
            return splitTag[0], splitTag[1]
        else:
            # we are a detached head not on a release tag so just treat
            # the raw describe like a topic branch name
            return _GetCommitRawDescription(), str(_GitCommitCount())
    else:
        # Get the version name from the branch name
        if _IsReleaseBranch(branchName):
            tailTag = _GetReleaseTailTag(branchName)
            return _GetReleaseVersionName(branchName), '{0}.{1}'.format(_GitBranchedCommitCount(tailTag), _GitCommitCount('HEAD', tailTag))
        else:
            return _sanitizeBranchName(branchName),  str(_GitCommitCount())


def _GitBranchedCommitCount(tailTag):
    """Returns what the build number was from the branch point"""
    prevRelTag = _GitPreviousReleaseTag(tailTag)
    prevCommitCount = ''

    if prevRelTag:
        prevCommitCount = prevRelTag.split('-')[1]
    else:
        prevCommitCount = str(_GitCommitCount(tailTag))

    return prevCommitCount


def _GitPreviousReleaseTag(tailTagName):
    """Looks for the tail tag and if it finds it then it looks for any release build tags
    that are also pointing to the same commit"""
    tags = _GetRepository().tags
    tailTag = None
    for tag in tags:
        if tag.name == tailTagName:
            tailTag = tag
            break

    if tailTag is None:
        return ''

    for tag in tags:
        if tag.name != tailTag.name:
            if  _IsReleaseBuildTag(tag.name):
                if  _GitTagRealCommitId(tag.name) == _GitTagRealCommitId(tailTag.name):
                    return tag.name
    
    return ''


def _GitTagRealCommitId(tagName):
    """use shell command to retreive commit id of where the tag points to"""
    # for some reason .hexsha was not returning the same id....
    return os.popen("git rev-list -n 1 '{0}'".format(tagName.replace("'", "'\"'\"'"))).read().strip()


def _IsReleaseBuildTag(tagName):
    """checks if the tag follows the pattern where if the
    tag is split on dash and the has at least two elements
    and the first two elements is a series of numbers delimited
    by dot and nothing else in those first two elements"""
    splitTag = tagName.split('-')
    if len(splitTag) > 1:
        return (re.match('^[0-9]+((\.[0-9]+)+)$', splitTag[0]) and re.match('^([0-9]+((\.[0-9]+)+)|[0-9]+)$', splitTag[1]))

    return False


def _IsReleaseBranch(branchName):
    """Check if the branch name is a release branch"""
    # a release branch begins with 'release_' and has
    # a base tag that matches the release name
    if branchName[:8] == 'release_':
        tailTag = _GetReleaseTailTag(branchName)
        if _gitHasTag(tailTag):
            return True
        else:
            # see if we can pull the tag down from any of the remotes
            repo = _GetRepository()
            for remote in repo.remotes:
                try:
                    remote.fetch('+refs/tags/{0}:refs/tags/{0}'.format(tailTag), None, **{'no-tags':True})
                except:
                    pass
            
            # try one more time after the fetch attempt(s)
            return (_gitHasTag(tailTag) is not None)
    else:
        return False


def _gitHasTag(tagName):
    """See if a tag exists in git"""
    tags =  _GetRepository().tags
    for tag in tags:
        if tag.name == tagName:
            return tag

    return None
 

def _sanitizeBranchName(branchName):
    """sanitize branch names to ensure some characters are not used"""
    return re.sub('[$?*`\\-"\'\\\\/\\s]', '_', branchName)


def _GetReleaseVersionName(branchName):
    """removes pre-pended 'release_' from branch name"""
    return branchName[8:]


def _GetReleaseTailTag(branchName):
    """removes pre-pended 'release_' from branch name"""
    return _GetReleaseVersionName(branchName) + '-RC1'


def _GitBranchName():
    """Returns current branch name or empty string"""
    try:
        return _GetRepository().active_branch.name
    except TypeError:
        return ''


def _IsGitDescribeFirstParentSupported():
    """Checks whether --first-parent parameter is valid for the
    version of git available"""
    
    try:
        repo = _GetRepository()
        repo.git.describe('--first-parent')
        return True
    except git.exc.GitCommandError:
        pass

    return False


def _GetCommitRawDescription():
    """Returns description of current commit"""

    args = ['--tags', '--match', '[0-9]*\.[0-9]*\.[0-9]*\-*']
    if _IsGitDescribeFirstParentSupported():
        args.insert(0, '--first-parent')

    repo = _GetRepository()
    raw = repo.git.describe(*args)
    raw = raw.rstrip()
    return raw


def _GetCurrentCommitReleaseTag():
    """If head is pointing to a relase tag it returns the name of the tag"""
    # If this condition hits, then we are currently on a tagged commit.
    tags = _GetRepository().tags
    headCommit = _GitTagRealCommitId('HEAD')
    for tag in tags:
        if _IsReleaseBuildTag(tag.name) and headCommit == _GitTagRealCommitId(tag.name):
            return tag.name
        else:
            pass
    
    return ''


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

    def get_warning_message(self):
        """Returns None, or a string describing known issues."""

        return None if _IsGitDescribeFirstParentSupported() else '''\
WARNING: Git version 1.8.4 or later is required to correctly determine the Open GEE version being built.
The Open GEE version is calculated from tags using the "git describe" command.
The "--first-parent" parameter introduced in Git 1.8.4 allows proper version calcuation on all branches.
Without the --first-parent parameter, the version calculated may be incorrect, depending on which branch is being built.
For information on upgrading Git, see:
https://github.com/google/earthenterprise/wiki/Frequently-Asked-Questions-(FAQs)#how-do-i-upgrade-git-to-the-recommended-version-for-building-google-earth-enterprise\
'''


# Exported variable for use by other modules:
open_gee_version = OpenGeeVersion()

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("-l", "--long", action="store_true", help="Output long format of version string")
    args = parser.parse_args()

    print open_gee_version.get_long() if args.long else open_gee_version.get_short()

    warning_message = open_gee_version.get_warning_message()
    if warning_message is not None:
        print >> sys.stderr, warning_message


__all__ = ['open_gee_version']

if __name__ == "__main__":
    main()
