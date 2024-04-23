Image_Folder=$1
Num_Frames=$2

for file in "$Image_Folder"/frame*.jpg; do
    [[ $file =~ frame([0-9]+)\.jpg ]] && (( ${BASH_REMATCH[1]} >= Num_Frames )) && rm "$file"
done
