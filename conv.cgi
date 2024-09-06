#!/usr/bin/perl
use strict;
use warnings;
use CGI;

$ENV{'LD_LIBRARY_PATH'} = '/usr/local/root/lib/root:' . ($ENV{'LD_LIBRARY_PATH'} // '');

my $cgi = CGI->new;
print $cgi->header('text/plain');

my $directory = '/var/www/html/JSROOT/EXP/TRIUMF/2024/ANA/'; # ディレクトリのパスを指定
my $command = "cd $directory && ./KAL/conv.sh load";

# コマンドを実行
my $output = `$command`;

# 結果を出力
print $output;

