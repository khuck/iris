#!/bin/bash
#SBATCH --job-name=gitlab-ci-dagger
#SBATCH --nodes=1
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=1
#SBATCH --output=slurm-test-out.txt
#SBATCH --error=slurm-test-err.txt
#SBATCH --exclusive
#SBATCH --partition=gpu
#SBATCH --nodelist=zenith

set -e

GIT_ROOT=$(git rev-parse --show-toplevel)
SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

echo Started: $(date)
echo Host: $(hostname)
echo Path: $GIT_ROOT
echo $SHELL
echo --------------------------------------------------------------------------------

which module
cd $GIT_ROOT
source $CONDA_ROOT/etc/profile.d/conda.sh
source /auto/software/iris/setup_system.source
source $IRIS_INSTALL_ROOT/setup.source
# source /auto/ciscratch/conda/etc/profile.d/conda.sh
cd apps/dagger
which conda
conda env create --force -p ./envs -f dagger.yaml
conda activate ./envs
./run-policy-evaluation.sh

grep -e '[E]' slurm-test-out.txt > errors.txt


echo --------------------------------------------------------------------------------
echo Finished: $(date)
echo Errors: $(wc -l errors.txt)
