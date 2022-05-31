# Argument 1: Location of qb64 (blank means current directory
# Argument 2: If not blank, qb64 will not be started after compiltaion

cd "$(dirname "$1")"
dont_run="$2"

Pause()
{
OLDCONFIG=`stty -g`
stty -icanon -echo min 1 time 0
dd count=1 2>/dev/null
stty $OLDCONFIG
}
echo "QB64 Setup"
echo ""

find . -name "*.command" -exec chmod +x {} \;

if [ -z "$(which clang++)" ]; then
  echo "Apple's C++ compiler not found."
  echo "Attempting to install Apple's Command Line Tools for Xcode..."
  echo "After installation is finished, run this setup script again."
  xcode-select --install
  [ -z "$dont_run" ] && Pause
  exit 1
fi

echo "Building 'QB64'"
make OS=osx clean
make OS=osx BUILD_QB64=y -j3

echo ""
if [ -f ./qb64 ]; then
  if [ -z "$dont_run" ]; then
    echo "Launching 'QB64'"
    ./qb64
  fi
  echo ""
  echo "Note: 'qb64' is located in same folder as this setup program."
  echo "Press any key to continue..."
  [ -z "$dont_run" ] && Pause
  exit 0
else
  echo "Compilation of QB64 failed!"
  [ -z "$dont_run" ] && Pause
  exit 1
fi
