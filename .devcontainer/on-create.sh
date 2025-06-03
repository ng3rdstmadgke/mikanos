#!/bin/bash
set -ex

cp ${PROJECT_DIR}/.devcontainer/conf/.tmux.conf ~/.tmux.conf

cat <<EOF >> ~/.bashrc

source ${PROJECT_DIR}/.devcontainer/.bashrc_private
EOF