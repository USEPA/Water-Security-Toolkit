#!/usr/bin/perl
#
# sp.script.pl <type-flag> <in-file> <measurement-file> <log-file>
#

use Text::ParseWords;

##
## A function that attempts to guess the type of value
##
sub type_of_value {
  my $text = $_[0];
  if ($text =~ / /) { return "text/string"; }
  if (!($text =~ /^\d/)) { return "text/string"; }
  if ($text =~ /\./) {return "numeric/double"; }
  return "numeric/integer";
}

##
## A function to process an option value, and interpret it as an
## environmental variable if it begins with '$'
##
sub process_value {
  my $word = $_[0];
  if (substr($word,0,1) eq "\$") {
     return $ENV{substr($word,1)};
  } else {
     return $word;
  }
}

##
## A function to process the input file, and return maps that summarize
## the option/value pairs.
##
sub process_input {
  my $filename = shift @_;
  #
  # Get input file
  #
  (open INPUT, "$filename") || die "Cannot open $filename\n";
  @lines = <INPUT>;
  close(INPUT);
  #
  # Parse each line, and form several maps:
  #    factor_name: number -> name
  #    factor: name -> value
  #    option: name -> value
  #
  foreach $line (@lines) {
    chomp($line);
    my @words = quotewords('\s+',0,$line);
    if ($#words == 1) {
       if (substr($words[0],0,8) eq "_factor_") {
          my @tokens = split(/_/, $words[0]);
          if ($tokens[3] eq "name") {
             ${@_[1]}{$tokens[2]} = $words[1];
          } elsif ($tokens[3] eq "value") {
             ${@_[2]}{ ${@_[1]}{$tokens[2]} } = process_value($words[1]);
          }
       }
       ${@_[0]}{$words[0]} = process_value($words[1]);
    } else {
       print "ERROR: bad option/value line\n";
       print "ERROR: $#words different tokens found\n";
       for $word (@words) {
         print "ERROR: $word\n";
       }
       print "ERROR: $line\n";
    }
  }
}


####
#### NOTE: THE REST OF THIS FILE IS CUSTOMIZED FOR THE PARTICULAR USE WITHIN SPOT.
#### THE PREVIOUS FUNCTIONS COULD PROBABLY BE USED IN ANY TESTING SCRIPT.
####

##
## A function to process the SP logfile and print out interesting statistics about the
## sensor placement solver's performance.
##
sub process_logfile {
  my $OUTPUT = shift @_;
  my $logfile = shift @_;

  (open INPUT, $logfile) || die "ERROR: $!! Cannot open file \"$logfile\"\n";
  @lines = <INPUT>;
  close(INPUT);
  #
  # Process file ... looking for stuff that starts with a line of ----'s
  #
  $flag=0;
  $status=0;
  foreach $line (@lines) {
    chomp($line);
    my @token = split(/\s+/,$line,$#line);
    if (($token[0] eq "CPU") && ($token[1] eq "RunTime=")) {
       print $OUTPUT "\"CPU RunTime\" numeric/double $token[2]\n";
       }
    elsif (($token[0] eq "CPU") && ($token[1] eq "TotalTime=")) {
       print $OUTPUT "\"CPU TotalTime\" numeric/double $token[2]\n";
       }
    elsif (($token[0] eq "WallClock") && ($token[1] eq "TotalTime=")) {
       print $OUTPUT "\"WallClock TotalTime\" numeric/double $token[2]\n";
       }
    elsif (($token[0] eq "Number") && ($token[2] eq "Contamination") && ($token[3] eq "Events:")) {
       print $OUTPUT "\"Number Events\" numeric/integer $token[4]\n";
       }
    elsif (($token[0] eq "Number") && ($token[2] eq "Contamination") && ($token[3] eq "Impacts:")) {
       print $OUTPUT "\"Number Impacts\" numeric/integer $token[4]\n";
       }
    elsif (($token[0] eq "Number") && ($token[2] eq "nonzeros:")) {
       print $OUTPUT "\"IP Nonzeros\" numeric/integer $token[3]\n";
       }
    elsif (($token[0] eq "Number") && ($token[2] eq "variables:")) {
       print $OUTPUT "\"IP Variables\" numeric/integer $token[3]\n";
       }
    elsif (($token[0] eq "Number") && ($token[2] eq "binary")) {
       print $OUTPUT "\"IP Binary Variables\" numeric/integer $token[4]\n";
       }
    elsif ($line eq "------------------------------------------------------------------------------") {
       if ($status == 0) { $status=1; }
       else { $status=0; }
    } elsif ($status == 1) {
      $flag=1;
      my @words = split(/:/,$line);
      if ($#words != 1) { next; }
      my $option = $words[0];
      $option =~ s/^\s+|\s+$//g;
      my $value = $words[1];
      $value =~ s/^\s+|\s+$//g;
      my $valuetype = type_of_value($value);
      if ($valuetype eq "text/string") {
         print $OUTPUT "\"$option\" $valuetype \"$value\"\n";
      } else {
         print $OUTPUT "\"$option\" $valuetype $value\n";
      }
    }
  }
  if ($flag) {
     print OUTPUT "sp_status text/string Pass\n";
  } else {
     print OUTPUT "sp_status text/string Fail\n";
  }
  print OUTPUT "exit_status numeric/integer 0\n";
}


##
## BEGIN MAIN
##
%option = ();
%factor_name = ();
%factor = ();
process_input($ARGV[1],\%option,\%factor_name,\%factor);

#
# Process each type separately
#
##
## TSO
##
if ($ARGV[0] eq "-tso") {
   $cmdline = "/reservoir1/sp_guidance/spot/bin/tevasim " . $factor{"tsg_file"} . " " . $factor{"tso_file"} . " " . $factor{"epanet_file"} . " > $ARGV[3]";
   print "Running command: $cmdline\n";
   `$cmdline > $ARGV[3]`;
   open OUTPUT, ">$ARGV[2]";
   print OUTPUT "_exit_status numeric/integer 0\n";
   close(OUTPUT);
}
##
## Impact
##
elsif ($ARGV[0] eq "-impact") {
   $cmdline = "/reservoir1/sp_guidance/spot/bin/tso2Impact " . $factor{"impact_prefix"} . " " . $factor{"objectives"} . " " . $factor{"scenario_aggr"} ."> $ARGV[3]";
   print "Running command: $cmdline\n";
   `$cmdline > $ARGV[3]`;
   open OUTPUT, ">$ARGV[2]";
   print OUTPUT "_exit_status numeric/integer 0\n";
   close(OUTPUT);
}

elsif ($ARGV[0] eq "-scenario_aggr") {
   $cmdline = "/reservoir1/sp_guidance/spot/bin/scenarioAggr " . $factor{"numEvents"} . " " . " " . $factor{"scenario_aggr"} ."> $ARGV[3]";
   print "Running command: $cmdline\n";
   `$cmdline > $ARGV[3]`;
   open OUTPUT, ">$ARGV[2]";
   print OUTPUT "_exit_status numeric/integer 0\n";
   close(OUTPUT);
}

elsif ($ARGV[0] eq "-mvscen") {
   $cmdline = "./mvscen " . $factor{"impact_prefix"} . "> $ARGV[3]";
   print "Running command: $cmdline\n";
   `$cmdline > $ARGV[3]`;
   open OUTPUT, ">$ARGV[2]";
   print OUTPUT "_exit_status numeric/integer 0\n";
   close(OUTPUT);
}
##
## SP-SIMPLE
##
elsif (($ARGV[0] eq "-sp-simple") || ($ARGV[0] eq "-sp") || ($ARGV[0] eq "-sp-timing") || ($ARGV[0] eq "-sp-study") || ($ARGV[0] eq "-mvscen")) {

   if ($ARGV[0] eq "-sp") {
     $cmdline = "/reservoir1/sp_guidance/spot/bin/sp --path=/reservoir1/sp_guidance/spot/bin --path=/reservoir1/sp_guidance/spot/packages/sp/mod --print-log " . $factor{"impact_prefix"} . " " . $factor{"goals"} . " " . $factor{"solver"};

   } elsif ($ARGV[0] eq "-sp-study") {
     $cmdline = "/reservoir1/sp_guidance/spot/bin/sp --path=/reservoir1/sp_guidance/spot/bin --path=/reservoir1/sp_guidance/spot/packages/sp/mod --print-log " . $factor{"impact_prefix"} . " " . $factor{"goals"} . " " . $factor{"solver"} . " " . $factor{"sensors"};
   } elsif ($ARGV[0] eq "-sp-simple") {
     $objective = $goal . "_" . $measure;
     $cmdline = "/reservoir1/sp_guidance/spot/bin/sp --path=/reservoir1/sp_guidance/spot/bin --path=/reservoir1/sp_guidance/spot/packages/sp/mod --print-log --objective=" . $factor{"goals"} . "_" . $factor{"measure"} . " " . $factor{"impact_prefix"} . " " . $factor{"sensors"} . " " . $factor{"costs"} . " " . $factor{"format"} . " " . $factor{"summary"} . " " . $factor{"solver"} . " --seed=" . $option{"seed"} . " " . $factor{"threshold"} . " " . $factor{"gamma"} . " " , $factor{"side_constraint"} . " " . $factor{"sensor-locations"};
     #$cmdline = "../../bin/sp --path=../../../bin --path=../../etc/mod --print-log --objective=" . $factor{"goals"} . "_" . $factor{"measure"} . " " . $factor{"impact_prefix"} . " " . $factor{"sensors"} . " " . $factor{"solver"} . " --seed=" . $option{"seed"};
   } else {
     $objective = $goal . "_" . $measure;
     $cmdline = "/reservoir1/sp_guidance/spot/bin/sp --path=/reservoir1/sp_guidance/spot../../bin --path=/reservoir1/sp_guidance/spot/packages/sp/mod --print-log --objective=" . $factor{"goals"} . "_" . $factor{"measure"} . " " . $factor{"impact_prefix"} . " " . $factor{"solver"} . " --seed=" . $option{"seed"};
   }
   print "Running command: $cmdline\n";
   `$cmdline > $ARGV[3]`;
   open OUTPUT, ">$ARGV[2]";
   process_logfile(\*OUTPUT, $ARGV[3]);
   close(OUTPUT);
}
