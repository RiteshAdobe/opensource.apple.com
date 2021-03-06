#! /usr/local/perl -w
package main;

sub BaseTests {

	my $CLASS = shift;
	
	# Insert your test code below, the Test module is use()ed here so read
	# its man page ( perldoc Test ) for help writing this test script.
	
	# Test bare number processing
	diag "tests with bare numbers" unless $ENV{PERL_CORE};
	$version = $CLASS->new(5.005_03);
	is ( "$version" , "5.005030" , '5.005_03 eq 5.5.30' );
	$version = $CLASS->new(1.23);
	is ( "$version" , "1.230" , '1.23 eq "1.230"' );
	
	# Test quoted number processing
	diag "tests with quoted numbers" unless $ENV{PERL_CORE};
	$version = $CLASS->new("5.005_03");
	is ( "$version" , "5.005_030" , '"5.005_03" eq "5.005_030"' );
	$version = $CLASS->new("v1.23");
	is ( "$version" , "v1.23.0" , '"v1.23" eq "v1.23.0"' );
	
	# Test stringify operator
	diag "tests with stringify" unless $ENV{PERL_CORE};
	$version = $CLASS->new("5.005");
	is ( "$version" , "5.005" , '5.005 eq "5.005"' );
	$version = $CLASS->new("5.006.001");
	is ( "$version" , "v5.6.1" , '5.006.001 eq v5.6.1' );
	$version = $CLASS->new("1.2.3_4");
	is ( "$version" , "v1.2.3_4" , 'alpha version 1.2.3_4 eq v1.2.3_4' );
	
	# test illegal formats
	diag "test illegal formats" unless $ENV{PERL_CORE};
	eval {my $version = $CLASS->new("1.2_3_4")};
	like($@, qr/multiple underscores/,
	    "Invalid version format (multiple underscores)");
	
	eval {my $version = $CLASS->new("1.2_3.4")};
	like($@, qr/underscores before decimal/,
	    "Invalid version format (underscores before decimal)");
	
	eval {my $version = $CLASS->new("1_2")};
	like($@, qr/alpha without decimal/,
	    "Invalid version format (alpha without decimal)");
	
	# for this first test, just upgrade the warn() to die()
	eval {
	    local $SIG{__WARN__} = sub { die $_[0] };
	    $version = $CLASS->new("1.2b3");
	};
	my $warnregex = "Version string '.+' contains invalid data; ".
		"ignoring: '.+'";

	like($@, qr/$warnregex/,
	    "Version string contains invalid data; ignoring");

        # from here on out capture the warning and test independently
	my $warning;
	local $SIG{__WARN__} = sub { $warning = $_[0] };
	$version = $CLASS->new("99 and 44/100 pure");

	like($warning, qr/$warnregex/,
	    "Version string contains invalid data; ignoring");
	ok ("$version" eq "99.000", '$version eq "99.000"');
	ok ($version->numify == 99.0, '$version->numify == 99.0');
	ok ($version->normal eq "v99.0.0", '$version->normal eq v99.0.0');
	
	$version = $CLASS->new("something");
	like($warning, qr/$warnregex/,
	    "Version string contains invalid data; ignoring");
	ok (defined $version, 'defined $version');
	
	# reset the test object to something reasonable
	$version = $CLASS->new("1.2.3");
	
	# Test boolean operator
	ok ($version, 'boolean');
	
	# Test class membership
	isa_ok ( $version, $CLASS );
	
	# Test comparison operators with self
	diag "tests with self" unless $ENV{PERL_CORE};
	ok ( $version eq $version, '$version eq $version' );
	is ( $version cmp $version, 0, '$version cmp $version == 0' );
	ok ( $version == $version, '$version == $version' );
	
	# test first with non-object
	$version = $CLASS->new("5.006.001");
	$new_version = "5.8.0";
	diag "tests with non-objects" unless $ENV{PERL_CORE};
	ok ( $version ne $new_version, '$version ne $new_version' );
	ok ( $version lt $new_version, '$version lt $new_version' );
	ok ( $new_version gt $version, '$new_version gt $version' );
	ok ( ref(\$new_version) eq 'SCALAR', 'no auto-upgrade');
	$new_version = "$version";
	ok ( $version eq $new_version, '$version eq $new_version' );
	ok ( $new_version eq $version, '$new_version eq $version' );
	
	# now test with existing object
	$new_version = $CLASS->new("5.8.0");
	diag "tests with objects" unless $ENV{PERL_CORE};
	ok ( $version ne $new_version, '$version ne $new_version' );
	ok ( $version lt $new_version, '$version lt $new_version' );
	ok ( $new_version gt $version, '$new_version gt $version' );
	$new_version = $CLASS->new("$version");
	ok ( $version eq $new_version, '$version eq $new_version' );
	
	# Test Numeric Comparison operators
	# test first with non-object
	$new_version = "5.8.0";
	diag "numeric tests with non-objects" unless $ENV{PERL_CORE};
	ok ( $version == $version, '$version == $version' );
	ok ( $version < $new_version, '$version < $new_version' );
	ok ( $new_version > $version, '$new_version > $version' );
	ok ( $version != $new_version, '$version != $new_version' );
	
	# now test with existing object
	$new_version = $CLASS->new($new_version);
	diag "numeric tests with objects" unless $ENV{PERL_CORE};
	ok ( $version < $new_version, '$version < $new_version' );
	ok ( $new_version > $version, '$new_version > $version' );
	ok ( $version != $new_version, '$version != $new_version' );
	
	# now test with actual numbers
	diag "numeric tests with numbers" unless $ENV{PERL_CORE};
	ok ( $version->numify() == 5.006001, '$version->numify() == 5.006001' );
	ok ( $version->numify() <= 5.006001, '$version->numify() <= 5.006001' );
	ok ( $version->numify() < 5.008, '$version->numify() < 5.008' );
	#ok ( $version->numify() > v5.005_02, '$version->numify() > 5.005_02' );
	
	# test with long decimals
	diag "Tests with extended decimal versions" unless $ENV{PERL_CORE};
	$version = $CLASS->new(1.002003);
	ok ( $version eq "1.2.3", '$version eq "1.2.3"');
	ok ( $version->numify == 1.002003, '$version->numify == 1.002003');
	$version = $CLASS->new("2002.09.30.1");
	ok ( $version eq "2002.9.30.1",'$version eq 2002.9.30.1');
	ok ( $version->numify == 2002.009030001,
	    '$version->numify == 2002.009030001');
	
	# now test with alpha version form with string
	$version = $CLASS->new("1.2.3");
	$new_version = "1.2.3_4";
	diag "tests with alpha-style non-objects" unless $ENV{PERL_CORE};
	ok ( $version lt $new_version, '$version lt $new_version' );
	ok ( $new_version gt $version, '$new_version gt $version' );
	ok ( $version ne $new_version, '$version ne $new_version' );
	
	$version = $CLASS->new("1.2.4");
	diag "numeric tests with alpha-style non-objects" unless $ENV{PERL_CORE};
	ok ( $version > $new_version, '$version > $new_version' );
	ok ( $new_version < $version, '$new_version < $version' );
	ok ( $version != $new_version, '$version != $new_version' );
	
	# now test with alpha version form with object
	$version = $CLASS->new("1.2.3");
	$new_version = $CLASS->new("1.2.3_4");
	diag "tests with alpha-style objects" unless $ENV{PERL_CORE};
	ok ( $version < $new_version, '$version < $new_version' );
	ok ( $new_version > $version, '$new_version > $version' );
	ok ( $version != $new_version, '$version != $new_version' );
	ok ( !$version->is_alpha, '!$version->is_alpha');
	ok ( $new_version->is_alpha, '$new_version->is_alpha');
	
	$version = $CLASS->new("1.2.4");
	diag "tests with alpha-style objects" unless $ENV{PERL_CORE};
	ok ( $version > $new_version, '$version > $new_version' );
	ok ( $new_version < $version, '$new_version < $version' );
	ok ( $version != $new_version, '$version != $new_version' );
	
	$version = $CLASS->new("1.2.3.4");
	$new_version = $CLASS->new("1.2.3_4");
	diag "tests with alpha-style objects with same subversion" unless $ENV{PERL_CORE};
	ok ( $version > $new_version, '$version > $new_version' );
	ok ( $new_version < $version, '$new_version < $version' );
	ok ( $version != $new_version, '$version != $new_version' );
	
	diag "test implicit [in]equality" unless $ENV{PERL_CORE};
	$version = $CLASS->new("v1.2.3");
	$new_version = $CLASS->new("1.2.3.0");
	ok ( $version == $new_version, '$version == $new_version' );
	$new_version = $CLASS->new("1.2.3_0");
	ok ( $version == $new_version, '$version == $new_version' );
	$new_version = $CLASS->new("1.2.3.1");
	ok ( $version < $new_version, '$version < $new_version' );
	$new_version = $CLASS->new("1.2.3_1");
	ok ( $version < $new_version, '$version < $new_version' );
	$new_version = $CLASS->new("1.1.999");
	ok ( $version > $new_version, '$version > $new_version' );
	
	# that which is not expressly permitted is forbidden
	diag "forbidden operations" unless $ENV{PERL_CORE};
	ok ( !eval { ++$version }, "noop ++" );
	ok ( !eval { --$version }, "noop --" );
	ok ( !eval { $version/1 }, "noop /" );
	ok ( !eval { $version*3 }, "noop *" );
	ok ( !eval { abs($version) }, "noop abs" );

	# test the qv() sub
	diag "testing qv" unless $ENV{PERL_CORE};
	$version = qv("1.2");
	ok ( $version eq "1.2.0", 'qv("1.2") eq "1.2.0"' );
	$version = qv(1.2);
	ok ( $version eq "1.2.0", 'qv(1.2) eq "1.2.0"' );
	isa_ok( qv('5.008'), $CLASS );

	# test creation from existing version object
	diag "create new from existing version" unless $ENV{PERL_CORE};
	ok (eval {$new_version = $CLASS->new($version)},
		"new from existing object");
	ok ($new_version == $version, "class->new($version) identical");
	$new_version = $version->new();
	isa_ok ($new_version, $CLASS );
	is ($new_version, "0.000", "version->new() doesn't clone");
	$new_version = $version->new("1.2.3");
	is ($new_version, "v1.2.3" , '$version->new("1.2.3") works too');

	# test the CVS revision mode
	diag "testing CVS Revision" unless $ENV{PERL_CORE};
	$version = new $CLASS qw$Revision: 1.2$;
	ok ( $version eq "1.2.0", 'qw$Revision: 1.2$ eq 1.2.0' );
	$version = new $CLASS qw$Revision: 1.2.3.4$;
	ok ( $version eq "1.2.3.4", 'qw$Revision: 1.2.3.4$ eq 1.2.3.4' );
	
	# test the CPAN style reduced significant digit form
	diag "testing CPAN-style versions" unless $ENV{PERL_CORE};
	$version = $CLASS->new("1.23_01");
	is ( "$version" , "1.23_0100", "CPAN-style alpha version" );
	ok ( $version > 1.23, "1.23_01 > 1.23");
	ok ( $version < 1.24, "1.23_01 < 1.24");

	# test reformed UNIVERSAL::VERSION
	diag "Replacement UNIVERSAL::VERSION tests" unless $ENV{PERL_CORE};
	
	# we know this file is here since we require it ourselves
	$version = $Test::More::VERSION;
	eval "use Test::More $version";
	unlike($@, qr/Test::More version $version/,
		'Replacement eval works with exact version');
	
	$version = $Test::More::VERSION+0.01; # this should fail even with old UNIVERSAL::VERSION
	eval "use Test::More $version";
	like($@, qr/Test::More version $version/,
		'Replacement eval works with incremented version');
	
	$version =~ s/\.0$//; #convert to string and remove trailing '.0'
	chop($version);	# shorten by 1 digit, should still succeed
	eval "use Test::More $version";
	unlike($@, qr/Test::More version $version/,
		'Replacement eval works with single digit');
	
	$version += 0.1; # this would fail with old UNIVERSAL::VERSION
	eval "use Test::More $version";
	like($@, qr/Test::More version $version/,
		'Replacement eval works with incremented digit');
	
SKIP: 	{
	    skip 'Cannot test v-strings with Perl < 5.8.1', 4
		    if $] < 5.008_001; 
	    diag "Tests with v-strings" unless $ENV{PERL_CORE};
	    $version = $CLASS->new(1.2.3);
	    ok("$version" eq "v1.2.3", '"$version" eq 1.2.3');
	    $version = $CLASS->new(1.0.0);
	    $new_version = $CLASS->new(1);
	    ok($version == $new_version, '$version == $new_version');
	    ok($version eq $new_version, '$version eq $new_version');
	    $version = qv(1.2.3);
	    ok("$version" eq "v1.2.3", 'v-string initialized qv()');
	}

	diag "Tests with real-world (malformed) data" unless $ENV{PERL_CORE};

	# trailing zero testing (reported by Andreas Koenig).
	$version = $CLASS->new("1");
	ok($version->numify eq "1.000", "trailing zeros preserved");
	$version = $CLASS->new("1.0");
	ok($version->numify eq "1.000", "trailing zeros preserved");
	$version = $CLASS->new("1.0.0");
	ok($version->numify eq "1.000000", "trailing zeros preserved");
	$version = $CLASS->new("1.0.0.0");
	ok($version->numify eq "1.000000000", "trailing zeros preserved");
	
	# leading zero testing (reported by Andreas Koenig).
	$version = $CLASS->new(".7");
	ok($version->numify eq "0.700", "leading zero inferred");

	# leading space testing (reported by Andreas Koenig).
	$version = $CLASS->new(" 1.7");
	ok($version->numify eq "1.700", "leading space ignored");

}

1;
