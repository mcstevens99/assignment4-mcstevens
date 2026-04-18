#!/bin/sh

# Configuration variables
DESC="AESD Socket Server"
NAME=aesdsocket
# Obtém o caminho absoluto do diretório onde o script está localizado
#DIR=$(dirname $(readlink -f "$0"))
DIR="/usr/bin"
DAEMON="$DIR/$NAME"
PIDFILE=/var/run/$NAME.pid
# The -d flag ensures the program forks into the background
DAEMON_ARGS="-d"

case "$1" in
  start)
    echo "Starting $DESC: $NAME"
    # --start: Start the process if it's not already running
    # --exec: Point to the binary location
    # -- -d: Pass the daemon argument to the binary
    start-stop-daemon --start --oknodo --exec $DAEMON -- $DAEMON_ARGS
    ;;
  stop)
    if ! pidof $NAME > /dev/null; then
        echo "$DESC não está em execução. Nada para parar."
    else
        echo "Stopping $DESC: $NAME"
        # --stop: Sends SIGTERM by default (which our code handles for cleanup)
        # --retry: Wait 5 seconds for process to exit before giving up
        start-stop-daemon --stop --oknodo --retry 5 --exec $DAEMON 2>/dev/null
    fi
    ;;
  *)
    echo "Usage: $0 {start|stop}"
    exit 1
    ;;
esac

exit 0