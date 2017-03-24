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

use lib $main::ProductDir;

use Product;
use RPMVersion;

BEGIN {
    # Load all the products
    foreach my $product
        (grep(chomp $_,
              `find $main::ProductDir -name \\*.product -print`))
    {
        require $product;
    }

    # Sort AllProductList by primary key and pretty name
    @Product::AllProductList = sort {
        my $cmp = $a->SortPrimaryKey() <=> $b->SortPrimaryKey();
        if ($cmp == 0) {
            $cmp = $a->PrettyName() cmp $b->PrettyName();
        }
        return $cmp;
    } @Product::AllProductList;


    # Populate the AllProductHash
    foreach my $product (@Product::AllProductList) {
        $Product::AllProductHash{ref($product)} = $product;
    }

    # Check for dependencies that don't exist
    foreach my $product (@Product::AllProductList) {
        next if ($product->IsDeprecated());
        foreach my $dep ($product->DependsList()) {
            my $depfound = 0;
            foreach my $p2 (@Product::AllProductList) {
                next if ($p2->IsDeprecated());
                if (UNIVERSAL::isa($p2, $dep)) {
                    $depfound = 1;
                    last;
                }
            }
            if (!$depfound) {
                die "PACKAGING ERROR: $key has unknown dependency: $dep\n";
            }
        }
    }
}

package BootstrapAllProducts;


sub ExtraChecks {
    # Check for multiples from an exclusive set
    my %found;
    foreach my $product (@Product::AllProductList) {
        next unless $product->AvailableInPackage();
        my $key = ref($product);
        foreach my $excl ($product->ExclusiveList()) {
            if (exists $found{$excl}) {
                die "PACKAGING ERROR: $key and $excl both in package\n";
            }
        }
        $found{$key} = 1;
    }


    # Make sure that none of our available packages are older
    # than what's already installed
    foreach my $product (@Product::AllProductList) {
        my $prodname  = ref($product);
        if ($product->AvailableInPackage()) {
            my $availver = $product->AvailableVersion();
            if (!defined $availver) {
                die "PACKAGING ERROR: $prodname doesn't report AvailVersion\n";
            }
            if ($product->IsInstalled()) {
                my $instver = $product->InstalledVersion();
                if (RPMVersion::Compare($instver, $availver) > 0) {
                    die
"Installed version of $prodname is newer than what is available.\n" .
"  installed: $instver\n" .
"  available: $availver\n";
                }
            }
        }
    }
    
}

sub DumpProductInfo {
    local $" = ", ";   # "
    foreach my $product (@Product::AllProductList) {
        my $key = ref($product);
        my $name = $product->PrettyName();
        my $version = $product->InstalledVersion();
        
        print $product->AvailableInPackage() ? "A" : "!";
        if (defined $version) {
            my $running = $product->IsRunning();
            print(($running ? "R" : " "),
                  " ($key) \"$name\"\n");
        } else {
            print "  ($key) \"$name\"\n";
        }
        if (defined $version) {
            print "        installed version: $version\n";
        }
        my @deps = $product->DependsList();
        if (@deps) {
            print "        dependencies: @deps\n";
        }
        my @excl = $product->ExclusiveList();
        if (@excl) {
            print "        excludes: @excl\n";
        }
        my @old = $product->DeprecatesList();
        if (@old) {
            print "        replaces: @old\n";
        }
    }
}


1;
