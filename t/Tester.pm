package Tester;

use strict;
use warnings;

use Cwd qw(cwd);
use POSIX;
use Redis;

sub new {
    my $class = shift;
    my $option = {data_dir => "./tmp.test-db-dir", @_};
    return bless {option => $option}, $class;
}

sub start {
    my $self = shift;
    my ($client, $retry_times) =(undef, 30);
    my $child_pid = fork();

    qx(rm -fr $self->{option}{data_dir});

    if ($child_pid) {
        $self->{'pid'} = $child_pid;
    } else {
        my $null_fd = POSIX::open("/dev/null", &POSIX::O_WRONLY);
        POSIX::dup2($null_fd, 1);
        qx(mkdir -p $self->{option}{data_dir});
        my @cmd = (cwd() . '/redis-leveldb', '-D', $self->{option}{data_dir});
        push @cmd, "-M", $self->{option}{db_number} if defined($self->{option}{db_number});
        exec @cmd or die "# can not start the server instance!";
    }

    while (!$client && $retry_times > 0) {
        eval {
            $client = Redis->new(server => "localhost:8323");
        };
        select(undef, undef, undef, 0.2);
        $retry_times--;
    }
    $client or die "# can not connect to server";
    $self->{'client'} = $client;
}

sub stop {
    my $self = shift;
    my $stop_cmd = "kill -INT " . $self->{'pid'};
    qx($stop_cmd) and die "# server stop error!";
    qx(rm -fr $self->{option}{data_dir});
}

sub client {
    my $self = shift;
    return $self->{'client'};
}

sub try {
    my $self = shift;
    my $cmd = shift;
    return eval {
        $self->{'client'}->$cmd(@_);
    };
}

1;
