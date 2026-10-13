#!/usr/bin/env bash
# Parent-only transfer AFTER review. SSH agent/config supplies credentials externally.
# Does not execute probes, install dependencies, alter services or overwrite prior bundle.
set -euo pipefail
P=$(cd -- "$(dirname -- "$0")" && pwd)
[[ $# == 2 || $# == 3 ]] || { echo 'usage: 08-deploy.sh SSH_DEST bellard|ng|v8 [/storage/apps/falllinie-js-probes]' >&2; exit 2; }
dest=$1; engine=$2; remote=${3:-/storage/apps/falllinie-js-probes}
[[ $dest =~ ^[A-Za-z0-9_][A-Za-z0-9_.@-]*$ ]] || exit 2
[[ $engine == bellard || $engine == ng || $engine == v8 ]] || exit 2
[[ $remote =~ ^/storage/apps/[A-Za-z0-9_/-]+$ && $remote != *..* ]] || exit 2
opts=(); if [[ -n ${PROBE_SSH_CONFIG:-} ]]; then opts=(-F "$PROBE_SSH_CONFIG"); fi
archive="$P/.work/packages/$engine-arm64.tar.gz"; test -f "$archive"
# Whitelisted client values intentionally form the remote command.
# shellcheck disable=SC2029
ssh "${opts[@]}" "$dest" "mkdir -p '$remote' && test ! -e '$remote/$engine' && test ! -e '$remote/$engine-arm64.tar.gz'"
scp "${opts[@]}" "$archive" "$dest:$remote/$engine-arm64.tar.gz"
# Whitelisted client values intentionally form the remote command.
# shellcheck disable=SC2029
ssh "${opts[@]}" "$dest" "tar -xzf '$remote/$engine-arm64.tar.gz' -C '$remote' && cd '$remote/$engine' && sha256sum -c SHA256SUMS"
echo "TRANSFER VERIFIED; no execution. Parent command: bash $remote/$engine/run-target.sh 300"
