#!/usr/bin/env bash
#-------------------------------------------------------------------
# test script for watchdog
source $TESTLIBS
LEADER_DIR=leader
STANDBY_DIR=standby
PSQL=$PGBIN/psql
success_count=0

rm -fr $LEADER_DIR
rm -fr $STANDBY_DIR

mkdir $LEADER_DIR
mkdir $STANDBY_DIR


# dir in leader directory
cd $LEADER_DIR

# create leader environment
echo -n "creating leader pgpool..."
$PGPOOL_SETUP -m n -n 1 -p 11000|| exit 1
echo "leader setup done."


# copy the configurations from to standby
cp -r etc ../$STANDBY_DIR/

source ./bashrc.ports
cat ../leader.conf >> etc/pgpool.conf
echo 0 > etc/pgbalancer_node_id

./startall
wait_for_pgpool_startup


# back to test root dir
cd ..


# create standby environment

mkdir $STANDBY_DIR/log
echo -n "creating standby pgpool..."
cat standby.conf >> $STANDBY_DIR/etc/pgpool.conf
# since we are using the same pgbalancer conf as of leader. so change the pid file path in standby pgpool conf
echo "pid_file_name = '$PWD/pgpool2.pid'" >> $STANDBY_DIR/etc/pgpool.conf
echo 1 > $STANDBY_DIR/etc/pgbalancer_node_id
# start the standby pgbalancer by hand
$PGPOOL_INSTALL_DIR/bin/pgpool -D -n -f $STANDBY_DIR/etc/pgpool.conf -F $STANDBY_DIR/etc/pcp.conf -a $STANDBY_DIR/etc/pool_hba.conf > $STANDBY_DIR/log/pgpool.log 2>&1 &

# First test check if both pgbalancer have found their correct place in watchdog cluster.
echo "Waiting for the pgpool leader..."
for i in 1 2 3 4 5 6 7 8 9 10
do
	grep "I am the cluster leader node. Starting escalation process" $LEADER_DIR/log/pgpool.log > /dev/null 2>&1
	if [ $? = 0 ];then
		success_count=$(( success_count + 1 ))
		echo "Leader brought up successfully."
		break;
	fi
	echo "[check] $i times"
	sleep 2
done

# now check if standby has successfully joined connected to the leader.
echo "Waiting for the standby to join cluster..."
for i in 1 2 3 4 5 6 7 8 9 10
do
	grep "successfully joined the watchdog cluster as standby node" $STANDBY_DIR/log/pgpool.log > /dev/null 2>&1
	if [ $? = 0 ];then
		success_count=$(( success_count + 1 ))
		echo "Standby successfully connected."
		break;
	fi
	echo "[check] $i times"
	sleep 2
done

# at this point the watchdog leader and stabdby are working.
# check to see if "pgpool reset" command works.
echo "Check pgpool reset command..."
for i in 11000 11100
do
    echo "Check pgpool reset command on port $i..."
    $PSQL -p $i -c "pgpool reset client_idle_limit" test
    if [ $? = 0 ];then
	success_count=$(( success_count + 1 ))
    fi
done

# step 2 stop leader pgpool and see if standby take over
$PGPOOL_INSTALL_DIR/bin/pgpool -f $LEADER_DIR/etc/pgpool.conf -m f stop

echo "Checking if the Standby pgbalancer detected the leader shutdown..."
for i in 1 2 3 4 5 6 7 8 9 10
do
	grep " is shutting down" $STANDBY_DIR/log/pgpool.log > /dev/null 2>&1
	if [ $? = 0 ];then
		success_count=$(( success_count + 1 ))
		echo "Leader shutdown detected."
		break;
	fi
	echo "[check] $i times"
	sleep 2
done

# Finally see if standby take over

echo "Checking if the Standby pgbalancer takes over the leader responsibility..."
for i in 1 2 3 4 5 6 7 8 9 10
do
	grep "I am the cluster leader node. Starting escalation process" $STANDBY_DIR/log/pgpool.log > /dev/null 2>&1
	if [ $? = 0 ];then
		success_count=$(( success_count + 1 ))
		echo "Standby successfully became the new leader."
		break;
	fi
	echo "[check] $i times"
	sleep 2
done

# we are done. Just stop the standby pgbalancer
$PGPOOL_INSTALL_DIR/bin/pgpool -f $STANDBY_DIR/etc/pgpool.conf -m f stop
cd leader
./shutdownall

echo "$success_count out of 6 successful";

if test $success_count -eq 6
then
    exit 0
fi

exit 1
