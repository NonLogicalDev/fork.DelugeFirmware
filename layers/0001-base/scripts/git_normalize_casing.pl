#!/usr/bin/env perl
use strict;
use warnings;
use bytes;

# Operate on Git's private index so case-colliding source paths never need a checkout.
my @renames = (
    [ 'contrib/sd_card/MIDI/KORG/', 'contrib/sd_card/MIDI/Korg/' ],
    [ 'contrib/sd_card/MIDI_DEVICES/DEFINITION/KORG/', 'contrib/sd_card/MIDI_DEVICES/DEFINITION/Korg/' ],
);

open my $listing, '-|', 'git', 'ls-files', '--stage', '-z' or die "Cannot list Git index: $!\n";
binmode $listing;
my (@entries, %paths, %removed);
{
    local $/ = "\0";
    while (defined(my $record = <$listing>)) {
        substr($record, -1, 1) eq "\0" or die "Malformed Git index listing\n";
        chop $record;
        my ($metadata, $path) = split /\t/, $record, 2;
        defined $path or die "Malformed Git index entry\n";
        my ($mode, $oid, $stage) = split / /, $metadata;
        defined $stage && $stage eq '0' or die "Cannot normalize an index containing unresolved entries\n";
        $paths{$path} = 1;
        for my $rename (@renames) {
            my ($old, $new) = @$rename;
            next unless index($path, $old) == 0;
            push @entries, [$mode, $oid, $path, $new . substr($path, length($old))];
            $removed{$path} = 1;
            last;
        }
    }
}
close $listing or die "Cannot list Git index\n";

# Reject ASCII case collisions before changing the index on case-insensitive filesystems.
sub folded {
    my ($path) = @_;
    $path =~ tr/A-Z/a-z/;
    return $path;
}
my (%remaining, %destinations);
for my $path (keys %paths) {
    $remaining{folded($path)} = 1 unless $removed{$path};
}
for my $entry (@entries) {
    my $path = folded($entry->[3]);
    if ($remaining{$path} || $destinations{$path}++) {
        die "Case normalization would collide with an existing index entry\n";
    }
}

if (@entries) {
    my $source_commit = $ENV{LAYERCTL_SOURCE_COMMIT} // die "LAYERCTL_SOURCE_COMMIT is required\n";
    my $zero = '0' x length($source_commit);
    open my $update, '|-', 'git', '-c', 'core.ignorecase=false', 'update-index', '-z', '--index-info'
        or die "Cannot update Git index: $!\n";
    binmode $update;
    for my $entry (@entries) {
        print {$update} "0 $zero\t$entry->[2]\0" or die "Cannot write Git index update: $!\n";
    }
    for my $entry (@entries) {
        print {$update} "$entry->[0] $entry->[1]\t$entry->[3]\0" or die "Cannot write Git index update: $!\n";
    }
    close $update or die "Cannot update Git index\n";
}

exec 'git', 'write-tree' or die "Cannot write Git tree: $!\n";
