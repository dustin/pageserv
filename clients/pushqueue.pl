#!/usr/bin/perl

$debug=1;

$sockaddr='S n a4 x8';
$hostname="irony";
# $hostname=~s/[\n\r]//g;

sub timeout
{
	print "Connection timed out.\n";
	exit(1);
}

sub openhost
{
local ($remote)=@_;
local ($portnum, $port, $name, $aliases, $proto, $type, $len);

	$portnum=1029;

	print "Portnum is $portnum\n" if($debug);
	($name, $aliases, $proto)=getprotobyname('tcp');
	($name, $aliases, $port)=getservbyname($portnum, 'tcp');

	($name, $aliases, $type, $len, $thisaddr)=gethostbyname($hostname);
	print "Looking up $remote\n" if ($debug);
	($name, $aliases, $type, $len, $thataddr)=gethostbyname($remote);
	if(!defined($name))
	{
		print "Unable to resolve remote hostname.\n";
		exit(1);
	}
	# print "Connecting to $name:$portnum\n";

	$me=pack($sockaddr, 2, 0, $thisaddr);
	$rem=pack($sockaddr, 2, $portnum, $thataddr);

	if(!socket(S, 2, 1, $proto))
	{
		print "Can't open socket: $!\n";
		exit(1);
	}

	if(!bind(S, $me))
	{
		print "Can't bind socket: $!\n";
		exit(1);
	}

	$rem =~ /(.*)/;
	if(!connect(S, $rem))
	{
		print "Can't connect: $!\n";
		exit(1);
	}

	select(S); $|=1; select('stdout'); $|=1;
}

$SIG{'ALRM'}= 'timeout';


# CORE
	$name=<>;
	$name=~s/[\n\r]//g;
	$message=<>;
	$message=~s/[\n\r]//g;

	alarm 20;
	&openhost("localhost");
	alarm 20;
	$_=<S>;
	/version ([0-9A-z\.]+)/;
	print "Remote system is running server version $1\n";
	print "sending mash\r\n" if($debug);
	print S "mash\r\n";
	print "sending $name\r\n" if($debug);
	print S "$name\r\n";
	print "sending $message\r\n" if($debug);
	print S "$message\r\n";
	print "sent \`\`$message\'\' to $name\r\n";
	@_=<S>;

	print "Shutting down...\n" if($debug);
	shutdown(S, 2);
