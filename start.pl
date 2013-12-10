#!/usr/bin/perl -w

#print "num:".@ARGV."\n";
if(@ARGV==0)
{
	print "please input prog name\n";
	exit;
}
$ps=$ARGV[0];
if(!(-e $ps) )
{
        print "program doesn't exist\n";
        exit;
}

$pidfile=$ps.".pid";
if((-e $pidfile) )
{
	print "program is running!!!\n";
	exit;
}
if(!($p2id=fork()))
{
	setpgrp(0,0);
	$SIG{QUIT}='sig_quit'; 
	$SIG{INT}='IGNORE';
	$SIG{TERM}='sig_quit'; 
	while(1)
	{
		if(!($p3id=fork))
		{
			open STDOUT,">/dev/null";
			open STDERR,">/dev/null";
			exec($ps);
			die $!;
		}
		wait;
		sleep(5);
	}
}
open FH,">$pidfile";
print $p2id,"\n";
print FH $p2id;
close FH;
sub sig_quit
{
	kill 2,$p3id;
	kill 15,$p3id;
	wait;
	unlink $pidfile;
	exit;
}
