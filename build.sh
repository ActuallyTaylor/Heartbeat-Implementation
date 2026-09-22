echo "[Unix/Linux Shell Mode]"
cmake -B build
cmake --build build
echo "build completed"
pids=()

./build/targets/bus/Bus &
pids+=($!)

sleep 1

for i in {1..3}; do
  ./build/targets/camera/Camera "$i" &
  pids+=($!)
done

./build/targets/monitor/Monitor &
pids+=($!)

cleanup() {
  echo ""
  echo "Stopping all processes..."

  kill "${pids[@]}" 2>/dev/null
  wait

  echo "All processes stopped."
  exit 0
}

trap cleanup SIGINT SIGTERM

wait
exit 0
