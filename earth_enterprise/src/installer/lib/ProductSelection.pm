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

package ProductSelection;

use strict;
use warnings;
use Product;
use TermHelps;
use Term::ANSIColor qw(:constants);


sub GuessInitial {
    my $prodsel = {};
    
    foreach my $product (@Product::AllProductList) {
        my $prodname = ref($product);
        if ($product->IsInstalled()) {
            if ($product->AvailableInPackage()) {
                $prodsel->{$prodname} = 1;
            } else {
                # let's try to find a product that is in the package
                # that can be used to replace this one
              replacementloop:
                foreach my $p2 (@Product::AllProductList) {
                    my $p2name = ref($p2);
                    next unless $p2->AvailableInPackage();
                    foreach my $excl ($p2->ExclusiveList()) {
                        if ($excl eq $prodname) {
                            $prodsel->{$p2name} = 1;
                            last replacementloop;
                        }
                    }
                    foreach my $dep ($p2->DeprecatesList()) {
                        if ($dep eq $prodname) {
                            $prodsel->{$p2name} = 1;
                            last replacementloop;
                        }
                    }
                }
            }
        }
    }

    return $prodsel;
}


sub PromptUser {
    my $prodsel = shift;


    my $want_clear = 1;

    while (1) {
        if ($want_clear) {
            TermHelps::Clear();
        }
        $want_clear = 1;
        print <<EOF;
Please select ALL of the products you want to run on this machine.
The products already installed are pre-selected below. Do NOT deselect them.
If you deselect a product below, that product will be uninstalled.
EOF

        my @used;
        my $count = 0;
        my ($prodname, $dep);
        foreach my $product (@Product::AllProductList) {
            if ($product->AvailableInPackage()) {
                ++$count;
                push @used, $product;
                $prodname = ref($product);
                if (exists $prodsel->{$prodname}) {
                    print "   [X]  ";
                } else {
                    print "   [ ]  ";
                }
                print(BOLD, $count, RESET, ": ",
                      $product->PrettyName(), "\n");
            }
        }
        print "\n";

        print(BOLD, "Please select 1-$count. Press F when finished: ", RESET);
        my $key;
        do {
            # beep if it's not the first time
            TermHelps::Beep() if (defined $key);
            $key = HotKey::ReadKey();
            if (($key eq 'f') || ($key eq 'F')) {
                print "F\n\n";
                return $prodsel;
            }
        } while (($key lt '1') || ($key gt "$count"));
        print "$key\n";

        {
            my $product = $used[$key - 1];
            $prodname = ref($product);
            if (exists $prodsel->{$prodname}) {
                # we want to deselect it
                # check other selected to see if this will affect them
                my @deps = FindSelectedThatDependOn($prodsel, $prodname);
                if (@deps) {
                    print "\n";
                    print "In order to deselect\n";
                    print "   ", $product->PrettyName(), "\n";
                    print "the following would also need to be deselected:\n";
                    foreach $dep (@deps) {
                        print("   ",
                              $Product::AllProductHash{$dep}->PrettyName(),
                              "\n");
                    }
                    print "\n";
                    if (!prompt::Confirm("Proceed with deselection", 'Y')) {
                        next;
                    }
                }
                delete $prodsel->{$prodname};
                foreach $dep (@deps) {
                    delete $prodsel->{$dep};
                }
            } else {
                my @deps = GetNeededInstall($product, $prodsel);

                my $ok = 1;
                # we want to select it
              depprod:
                foreach my $depprod (@deps) {
                    foreach my $excl ($depprod->InstallExclusiveList()) {
                        if (exists $prodsel->{$excl}) {
                            print("\n", RED, $depprod->PrettyName(),
                                  "\nand\n",
                                  $Product::AllProductHash{$excl}->PrettyName(),
                                  "\ncannot both be selected\n\n", RESET);
                            prompt::PressAnyKeyToContinue();
                            $ok = 0;
                            last depprod;
                        }
                    }
                }

                # It's safe, go ahead and select the required products
                if ($ok) {
                    foreach my $depprod (@deps) {
                        $prodsel->{ref($depprod)} = 1;
                    }
                }
            }
        }
    }



    return $prodsel;
}


sub FindSelectedThatDependOn {
    my ($prodsel, $target) = @_;
    my @ret;
    foreach my $product (@Product::AllProductList) {
        my $prodname = ref($product);
        if (exists $prodsel->{$prodname}) {
            foreach my $dep ($product->DependsList()) {
                if ($target->UNIVERSAL::isa($dep)) {
                    push @ret, $prodname;
                    last;
                }
            }
        }
    }
    return @ret;
}

sub FindAvailableDependenciesFor {
    my ($target) = @_;
    my @ret;
    foreach my $dep ($target->DependsList()) {
        foreach my $product (@Product::AllProductList) {
            if ($product->AvailableInPackage()) {
                if ($product->UNIVERSAL::isa($dep)) {
                    push @ret, $product;
                    last;
                }
            }
        }
    }
    return @ret;
}


sub GetNeededInstall {
    my ($product, $prodsel) = @_;
    my %hash;

    GetNeededInstallRecursive($product, \%hash);
    my @notinstalled;
    foreach my $key (keys %hash) {
        if (!exists $prodsel->{$key}) {
            push @notinstalled, $hash{$key};
        }
    }
    return @notinstalled;
}

sub GetNeededInstallRecursive {
    my ($product, $hash) = @_;
    my $prodname = ref($product);

    if (! exists $hash->{$prodname}) {
        $hash->{$prodname} = $product;
        my @deps = FindAvailableDependenciesFor($product);
        foreach my $dep (@deps) {
            GetNeededInstallRecursive($dep, $hash);
        }
    }
}



1;
