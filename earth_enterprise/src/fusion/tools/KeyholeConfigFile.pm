# -*- Perl -*-
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

package KeyholeConfigFile;

use strict;
use IO::Handle;
use Storable;

sub new
{
    my $class = shift;
    my $filename = shift;
    my $self = { };
    bless $self, $class;
 
    $self->Load($filename) if (defined $filename);
 
    return $self;
}

sub FindSourceId
{
    my $self = shift;
    my $filename = shift;

    if (exists $self->{Sources}) {
        foreach my $key (keys %{$self->{Sources}}) {
            next if $key =~ /^-/;
            return $key if $self->{Sources}->{$key}->{Name} eq $filename;
        }
    }
    return undef;
}


sub max($$) {
    my ($l, $r) = @_;
    return $l > $r ? $l : $r;
}

sub FindMaxStyleId
{
    my $self = shift;
    my $maxstyle = 0;
    foreach my $layer (grep(ref($_) eq "HASH", values %{$self->{Layers}})) {
        foreach my $select (grep(ref($_) eq "HASH", values %{$layer->{Select}})) {
            foreach my $filter (grep(ref($_) eq "HASH", values %{$select->{Filters}})) {
                if (exists $filter->{Feature} &&
                    exists $filter->{Feature}->{Style}) {
                    $maxstyle = max($maxstyle, $filter->{Feature}->{Style}->{ID});
                }
                if (exists $filter->{Site} &&
                    exists $filter->{Site}->{Style}) {
                    $maxstyle = max($maxstyle, $filter->{Site}->{Style}->{ID});
                }
            }
        }
    }
    return $maxstyle;
}


sub AddSource
{
    my $self = shift;
    my $filename = shift;

    $self->{Sources} = { } unless exists $self->{Sources};

    my $newid = $self->FindSourceId($filename);
    if (! defined $newid) {
        $newid = _FindNextId($self->{Sources});
        _AddChild($self->{Sources}, $newid, { Name => $filename, ModTime => -1 });
    }
    return $newid;
}

sub AddSourceToLayer
{
    my $self = shift;
    my $sourcefile   = shift;
    my $layerid = shift;
    my $selectid = shift;
 
    # find layer and select
    my $layer = $self->{Layers}->{$layerid};
    die "Unable to find layer $layerid\n" unless defined $layer;
    my $select = $layer->{Select}->{$selectid};
    die "Layer $layerid doesn't have a select $selectid\n"
        unless defined $select;

    # add source to Sources section
    my $sid = $self->AddSource($sourcefile);

    # add the source to the layer
    my $newid = _FindNextId($layer->{Select});
    my $newSelect = Storable::dclone($select);
    KeyholeConfigFile::_AddChild($layer->{Select}, $newid, $newSelect);
    $newSelect->{Source} = $sid;

    # clean up KGBName & style id's
    foreach my $filter (grep(ref($_) eq "HASH",
                                 values %{$newSelect->{Filters}})) {
        if (exists $filter->{Feature}) {
            delete $filter->{Feature}->{KGBName};
            if (exists $filter->{Feature}->{Style}) {
                $filter->{Feature}->{Style}->{ID} = ++$self->{-maxstyle};
            }
        }
        if (exists $filter->{Site}) {
            delete $filter->{Site}->{KGBName};
            if (exists $filter->{Site}->{Style}) {
                $filter->{Site}->{Style}->{ID} = ++$self->{-maxstyle};
            }
        }
    }

    return $newid;
}
    

sub Load 
{
    my $self = shift;
    my $filename = shift;

    $self->{-filename} = $filename;

    my $in;
    open($in, $filename) || die "Unable to open KeyholeConfigFile $filename: $!\n";
    _SlurpLines($in, $self, 0);
    $self->{-maxstyle} = $self->FindMaxStyleId();

    close($in);
}


sub Save
{
    my $self = shift;
    my $filename = shift;
    $filename = $self->{-filename} unless defined $filename;

    die "Can't save KeyholeConfigFile w/o a filename\n" unless defined $filename;
    # save off a copy before overwriting it
    rename($filename, $filename.'~');

    my $out;
    open($out, ">$filename") || die "Unable to open output $filename: $!\n";
    _DumpLines($out, $self, 0);
    close($out);
}


sub _SlurpLines
{
    my $fh = shift;
    my $hash = shift;
    my $needclose = shift;
    my $startline = defined(IO::Handle->input_line_number($fh)) ?
        IO::Handle->input_line_number($fh) - 1 : 0;

    while (<$fh>) {
        chomp;
        if (/^\s*\}\s*$/) {
            die "Unexpected '}' for block starting on line $startline\n" unless $needclose;
            return;
        } elsif (/^\s*([^\s:]+)\s*:\s*"(.*)"\s*$/) {
            _AddChild($hash, $1, $2);
        } elsif (/^\s*([^\s:]+)\s*:\s*"(.*)$/) {  # "
            my $key = $1;
            my $str = $2;
            while (<$fh>) {
                chomp;
                if (/^(.*)(?<!")"\s*$/) {
                    $str .= "\n";
                    $str .= $1;
                    last;
                } else {
                    $str .= "\n";
                    $str .= $_;
                }
            }
            _AddChild($hash, $key, $str);
        } elsif (/^\s*([^\s\{]+)\s*\{\s*$/) {
            _AddChild($hash, $1, { });
            _SlurpLines($fh, $hash->{$1}, 1);
        }
    }
    die "Missing '}' for block starting on line $startline\n" if $needclose;
}

sub _DumpLines
{
    my $fh = shift;
    my $hash = shift;
    my $level = shift;

    # make sure all keys are in sortorder
    my $key;
    foreach $key (sort keys %{$hash}) {
        next if $key =~ /^-/;
        if (!grep($_ eq $key, @{$hash->{-sortorder}})) {
            push @{$hash->{-sortorder}}, $key;
        }
    }
    foreach $key (@{$hash->{-sortorder}}) {
        if (exists $hash->{$key}) {
            print $fh '  'x$level, $key;
            if (ref($hash->{$key}) eq "HASH") {
                print $fh " {\n";
                _DumpLines($fh, $hash->{$key}, $level+1);
                print $fh '  'x$level, "}\n";
            } else {
                print $fh " : \"", $hash->{$key}, "\"\n";
            }
        }
    }
}

sub _AddChild
{
    my $hash = shift;
    my $key  = shift;
    my $value = shift;

    die "Duplicate child key '$key'\n" if exists $hash->{$key};
    $hash->{$key} = $value;
    push @{$hash->{-sortorder}}, $key;
}

sub _FindNextId
{
    my $hash = shift;
    my $nextid;
    if (scalar(keys %{$hash})) {
        $nextid = 1 + (sort { $b <=> $a } grep(/^[^-]/, keys %{$hash}))[0];
    } else {
        $nextid = 1;
    }
    return $nextid;
}

return 1;
