if [ -z "$1" ]
  then
    echo "Usage: $0 <.c file1 to build .so from> <.c file2 to build .so from> ..."
fi

for name in "$@"
do
  name="${name::${#name}-2}"
  gcc -c -fPIC -o "$name.o" "$1" && gcc -shared -fPIC -o "$name.so" "$name.o" && rm "$name.o" && echo "Built $name.so!"
done
