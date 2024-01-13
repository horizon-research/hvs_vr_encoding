mkdir uncompressed_ip_repo
for file in *.zip; do
    # create a directory using the zip file name
    dir="${file%.*}"
    mkdir -p uncompressed_ip_repo/"$dir"

    # unzip the file into the directory
    unzip "$file" -d uncompressed_ip_repo/"$dir"
done