#!/bin/bash
args=("$@")         # copy them into an array

for ((i=0; i<${#args[@]}; i++)); do
	if [ ${args[i]} = "-l" ]; then
		PPATH="${args[i+1]}"
    fi
	if [ ${args[i]} = "-c" ]; then
		COMMAND="${args[i+1]}"
		if [ $COMMAND = "size" ]; then
			SIZE="${args[i+2]}"
    	fi
	fi

done
# echo "PPATH    = ${PPATH}"
# echo "COMMAND    = ${COMMAND}"
# echo "SIZE  = ${SIZE}"


if [ "$COMMAND" = "list" ]; then
	find "$PPATH" -type d #-links 2
fi

if [ "$COMMAND" = "size" ]; then
	if ! [[ "$SIZE" =~ ^[0-9]+$ ]]; then
		du -h "$PPATH" | sort -n 
	else
		du -h -a "$PPATH" | sort -n | tail -n "$SIZE"
	fi	
fi

if [ "$COMMAND" = "purge" ]; then
	rm -rf "$PPATH"
fi




