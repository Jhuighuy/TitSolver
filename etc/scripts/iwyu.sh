#!/bin/zsh

if ! command -v include-what-you-use &> /dev/null
then
  echo "include-what-you-use was not found."
  exit 0
fi

SCRIPTPATH="$(cd -- "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P)"

function check_file() {
  echo ""
  echo "===============" $1 "==============="
  echo ""
  include-what-you-use \
    -Xiwyu --error \
    -Xiwyu --no_fwd_decls \
    -Xiwyu --mapping_file=$SCRIPTPATH/iwyu.imp \
    -DTIT_IWYU=1 \
    -std=gnu++2b -stdlib=libc++ \
    -I/opt/homebrew/include -I./src $1
}

find $1 -name "*.hpp" | while read fname; do
  check_file $fname || exit $?
done
