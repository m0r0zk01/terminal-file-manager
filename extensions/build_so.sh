if [ -z "$1" ]
  then
    echo "Usage: $0 <.c file to build .so from>"
fi

name="$1"
name="${name::${#name}-2}"
gcc -c -o "$name.o" "$1" && gcc -shared -o "$name.so" "$name.o" && rm "$name.o" && echo "Built $name.so!"
