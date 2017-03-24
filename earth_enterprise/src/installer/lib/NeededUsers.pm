#
# Copyright 2017 Google Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

package NeededUsers;

use strict;
use warnings;
use Users;
use Product;
use prompt;
use TermHelps;


sub new {
    my ($class, $prodsel) = @_;
    my %checked;
    my %gids;
    my $self = bless {}, $class;
    $self->Query($prodsel);
    return $self;
}

sub Query {
    my ($self, $prodsel) = @_;

    # rest the lists to empty
    $self->{missinggroups} = [];
    $self->{missingusers} = [];
    $self->{brokenusers} = [];

    my %checked;
    my %gids;
    foreach my $prodname (keys %{$prodsel}) {
        my $product = $Product::AllProductHash{$prodname};
        foreach my $user ($product->RequiredUsers()) {
            my $key = join(':', @{$user});
            if (!exists $checked{$key}) {
                $checked{$key} = 1;

                my ($username, $groupname) = @{$user};
                if (!exists $gids{$groupname}) {
                    $gids{$groupname} = Users::GetGroupId($groupname);
                    if (!defined $gids{$groupname}) {
                        push @{$self->{missinggroups}}, $groupname;
                    }
                }
                my ($uid, $gid) = Users::GetUserAndGroupId($username);
                if (!defined $uid) {
                    push @{$self->{missingusers}}, $user;
                } elsif (!defined($gid) ||
                         !defined($gids{$groupname}) ||
                         ($gid != $gids{$groupname})) {
                    push @{$self->{brokenusers}}, $user;
                }
            }
        }
    }
}

sub Dump {
    my $self = shift;

    if (@{$self->{missinggroups}}) {
        print "Missing groups:\n";
        foreach my $group (@{$self->{missinggroups}}) {
            print "   $group\n";
        }
    }
    if (@{$self->{missingusers}}) {
        print "Missing users\n";
        foreach my $user (@{$self->{missingusers}}) {
            print "   ", join(':', @{$user}), "\n";
        }
    }
    if (@{$self->{brokenusers}}) {
        print "Users with wrong primary group:\n";
        foreach my $user (@{$self->{brokenusers}}) {
            my ($uid, $gid) = Users::GetUserAndGroupId($user->[0]);
            my $badgroup = Users::GetGroupName($gid);
            print "   ", join(':', @{$user}), " (currently group $badgroup) \n";
        }
    }
}

sub Empty {
    my $self = shift;

    return ((@{$self->{missinggroups}} == 0) &&
            (@{$self->{missingusers}} == 0) &&
            (@{$self->{brokenusers}} == 0));
}

sub FixOrDie {
    my ($self, $prodsel) = @_;

    while (!$self->Empty()) {
        TermHelps::Clear();
        print <<EOF;
The products you have selected require some users and/or groups that do not
currently exist on this machine. You will now have the opportunity to create
them yourself or have this installer create them for you. If you choose to
have the installer create them for you, you will be allowed to specify the
uid/gid.

EOF

        $self->Dump();
        print "\n";
        if (prompt::Confirm("Do you want this installer to create them")) {
            # will update the Query itself, no need for us to do it again
            $self->InteractiveFix($prodsel);
        } else {
            print <<EOF;

Please add or repair the users/groups listed above and press any key.
EOF
            HotKey::WaitForAnyKey();
            $self->Query($prodsel);
        }
    }
}


sub InteractiveFix {
    my ($self, $prodsel) = @_;

    while (!$self->Empty()) {
        if (@{$self->{missinggroups}}) {
            print("\nCreating missing groups ...\n");
            print("Leave gid empty to have it auto-assigned.\n");
            foreach my $groupname (@{$self->{missinggroups}}) {
                my $gid =
                    prompt::GetNumber("Please specify the gid for group '$groupname'",
                                      '');
                
                if (!Users::CreateGroup($groupname, $gid)) {
                    die "Unable to create group '$groupname'\n";
                }
            }

            # since we added some groups, we should re-query to see what
            # is broken. Maybe adding this group fixed the remaining
            # broken users
            $self->Query($prodsel);
            next;
        }
        if (@{$self->{missingusers}}) {
            print("\nCreating missing users ...\n");
            print("Leave uid empty to have it auto-assigned.\n");
            foreach my $user (@{$self->{missingusers}}) {
                my ($username, $groupname) = @{$user};
                my $uid =
                    prompt::GetNumber("Please specify the uid for user '$username'",
                                      '');
                
                if (!Users::CreateUser($username, $groupname, $uid)) {
                    die "Unable to create user '$username'\n";
                }
            }
        }
        if (@{$self->{brokenusers}}) {
            print("\nFixing users w/ wrong primary group ...\n");
            foreach my $user (@{$self->{brokenusers}}) {
                my ($username, $groupname) = @{$user};
                if (!Users::FixUserGroup($username, $groupname)) {
                    die "Unable to create user '$username'\n";
                }
            }
        }
        
        # requery to make sure we're done
        $self->Query($prodsel);
    }
}



1;
