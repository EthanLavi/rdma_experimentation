from absl import app, flags
from multiprocessing import Process
import subprocess
import os
from typing import List
import csv
import json

def domain_name(nodetype):
    """Function to get domain name"""
    node_i = ['r320',           'luigi',          'r6525',               'xl170',            'c6525-100g',       'c6525-25g',        'd6515']
    node_h = ['apt.emulab.net', 'cse.lehigh.edu', 'clemson.cloudlab.us', 'utah.cloudlab.us', 'utah.cloudlab.us', 'utah.cloudlab.us', 'utah.cloudlab.us']
    return node_h[node_i.index(nodetype)]



# Define FLAGS to represet the flags
FLAGS = flags.FLAGS

# Experiment configuration
flags.DEFINE_string('ssh_keyfile', '~/.ssh/id_rsa', 'Path to ssh file for authentication')
flags.DEFINE_string('ssh_user', 'esl225', 'Username for login')
flags.DEFINE_string('nodefile', '../rome/scripts/nodefiles/r320.csv', 'Path to csv with the node names')
flags.DEFINE_string('experiment_name', None, 'Used as local save directory', required=True)
flags.DEFINE_string('bin_dir', 'RDMA/rdma_iht', 'Directory where we run bazel build from')
flags.DEFINE_bool('dry_run', required=True, default=None, help='Print the commands instead of running them')

# Program run-types
flags.DEFINE_bool('send_bulk', required=False, default=False, help="Whether to run bulk operations (more for benchmarking)")
flags.DEFINE_bool('send_test', required=False, default=False, help="Whether to run basic method testing")
flags.DEFINE_bool('send_exp', required=False, default=False, help="Whether to run an experiment")
flags.DEFINE_enum('log_level', 'info', ['info', 'debug', 'trace'], 'The level of print-out in the program')

SINGLE_QUOTE = "'\"'\"'"
def make_one_line(proto):
    return SINGLE_QUOTE + ' '.join(line for line in str(proto).split('\n')) + SINGLE_QUOTE 

def quote(string):
    return f"'{string}'"

def is_valid(string):
    """Determines if a string is a valid experiment name"""
    for letter in string:
        if not letter.isalpha() and letter not in [str(i) for i in range(10)] and letter != "_":
            return False
    return True

def execute(commands):
    """For each command in commands, start a process"""
    # Create a function that will create a file and run the given command using that file as stout
    def __run__(cmd, outfile):
        with open(f"{outfile}.txt", "w+") as f:
            if FLAGS.dry_run:
                print(cmd)
            else:
                try:
                    subprocess.run(cmd, shell=True, check=True, stderr=f, stdout=f)
                    print("Successful Startup")
                    return
                except subprocess.CalledProcessError:
                    print("Invalid Startup")

    processes: List[Process] = []
    for cmd, file in commands:
        # Start a thread
        processes.append(Process(target=__run__, args=(cmd, os.path.join("results", FLAGS.experiment_name, file))))
        processes[-1].start()

    # Wait for all threads to finish
    for process in processes:
        process.join()


def main(args):
    # Simple input validation
    if not is_valid(FLAGS.experiment_name):
        print("Invalid Experiment Name")
        exit(1)
    print("Starting Experiment")
    # Create results directory
    os.makedirs(os.path.join("results", FLAGS.experiment_name), exist_ok=True)
    commands = []
    with open(FLAGS.nodefile, "r") as f:
        for node in csv.reader(f):
            # For every node in nodefile, get the node info
            nodename, nodealias, nodetype = node
            # Construct ssh command and payload
            ssh_login = f"ssh -i {FLAGS.ssh_keyfile} {FLAGS.ssh_user}@{nodealias}.{domain_name(nodetype)}"
            bazel_path = f"/users/{FLAGS.ssh_user}/go/bin/bazelisk"
            payload = f"cd {FLAGS.bin_dir}; {bazel_path} run main --log_level={FLAGS.log_level} --"
            # Adding run-type
            if FLAGS.send_test:
                payload += " --send_test"
            elif FLAGS.send_bulk:
                payload += " --send_bulk"
            elif FLAGS.send_exp:
                payload += " --send_exp"
            else:
                print("Must specify whether testing methods '--send_test', doing bulk operations '--send_bulk', or sending experiment '--send_exp'")
                exit(1)
            # Tuple: (Creating Command | Output File Name)
            commands.append((' '.join([ssh_login, quote(payload)]), nodename))
    # Execute the commands and let us know we've finished
    execute(commands)
    print("Finished Experiment")


if __name__ == "__main__":
    # Run abseil app
    app.run(main)