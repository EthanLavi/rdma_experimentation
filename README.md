# Venice

An attempt at fixing loopback. This project is more for the learning benefits and isn't currently intended to be used by anyone else.

## Origins

The name 'Venice' originated because it is loosely based on Rome. 


## Deploying

### Normal Deploy

1. Check availability
2. Create experiment
3. Select Ubuntu 20.04 (OS), r320 (Node Type), name (Experiment Name), hours (Duration)
4. Edit ./scripts/nodefiles/r320.csv with data from listview
5. conda activate rdma. And then wait while configuring.
6. cd into ./scripts
7. [ONCE FINISHED] Run sync command to check availability
```{bash}
python rexec.py --nodefile=nodefiles/r320.csv  --remote_user=esl225 --remote_root=/users/esl225/venice --local_root=/home/manager/Research/venice --sync
```
8. Check logs at /tmp/venice/logs for success
9. Run start up script
```{bash}
python rexec.py --nodefile=nodefiles/r320.csv --remote_user=esl225 --remote_root=/users/esl225/venice --local_root=/home/manager/Research/venice --sync --cmd="cd venice/scripts/setup && python3 run.py --resources all && sudo apt install perftest -y"
```
10. Wait while configuring. Can check /tmp/venice/logs for updates.
11. [ONCE FINISHED] Login to nodes or continue to run C&C using launch.py
```{bash}
python launch.py --experiment_name={exp} --nodry_run
```

## GDB

bazel build main --compilation_mode=dbg

gdb bazel-bin/main