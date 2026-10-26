#!/usr/bin/env bash
set -euo pipefail
ulimit -l unlimited
source_dir="$RUNNER_TEMP/3fs"
cluster=/tmp/opendal-3fs-cluster
# Select the software RDMA device even if the VM also exposes a hardware device.
sed -i '/\[ib_devices\]/a device_filter = ["opendal_rxe"]' "$source_dir"/tests/fuse/config/*launcher.toml "$source_dir"/tests/fuse/config/admin_cli.toml
bash "$source_dir/tests/fuse/run.sh" "$source_dir/build/bin" "$cluster" > /tmp/opendal-3fs-cluster.stdout 2>&1 &
cluster_pid=$!
trap 'kill "$cluster_pid" 2>/dev/null || true; wait "$cluster_pid" || true' EXIT
for _ in $(seq 1 120); do
  if mountpoint -q "$cluster/mnt" && [ -d "$cluster/mnt/test" ]; then break; fi
  kill -0 "$cluster_pid"
  sleep 1
done
mountpoint "$cluster/mnt"
library="$source_dir/build/src/lib/api"
cc -Wall -Wextra -Werror -I "$source_dir/src/lib/api" smoke.c -L "$library" -Wl,-rpath,"$library" -lhf3fs_api_shared -o /tmp/usrbio-smoke
timeout 60 /tmp/usrbio-smoke "$cluster/mnt" "$cluster/mnt/test/usrbio"
