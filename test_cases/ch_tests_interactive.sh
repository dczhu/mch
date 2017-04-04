ch_tests()
{
	Q()
	{
		local a
		while true; do
			read -p "===[$((Y+N+1)) / $NUM_TESTS]=== Is this expected? [y/n]: " a

			if [ "$a" == "y" ] || [ "$a" == "Y" ]; then
				let Y++
				break
			elif [ "$a" == "n" ] || [ "$a" == "N" ]; then
				FAIL[$N]="$LINE"
				let N++
				break
			fi
		done
	}

	[ ! -f "$1" ] && { echo "File \"$1\" doesn't exist"! ; echo "Usage: $FUNCNAME \$test_case_file"; return 1; }

	echo "-------------------------------------------------------------------------------"

	local NUM_TESTS=$(cat $1 | /bin/grep -P -v "^[ \t]*$|^[ \t]*#" | wc -l)

	local Y=0
	local N=0
	local LINE LINEP
	local -a FAIL

	# Read from file (LINE) ***AND*** stdin (Q)
	# Use raw mode of the built-in read.
	while read -r LINE <&3; do
		echo "$LINE"

		if [ "${LINE//[ 	]/}" == "" ] || [ "$(echo "${LINE}" | sed -e "s/^[ \t]*#//")" != "${LINE}" ]; then
			continue
		fi

		eval "$LINE"

		# To test h, replace ' ' with | and substitute grep -P for h.
		LINEP="$LINE"
		LINEP="${LINEP//| h/| /bin/grep -P --color=always}"
		LINEP="${LINEP//| cxpgrep?/| /bin/grep -P --color=always }"
		LINEP="${LINEP//\' \'/|}"
		LINEP="${LINEP//\" \"/|}"
		if [ "$LINEP" != "$LINE" ]; then
			echo "$LINEP"

			eval "$LINEP"
		fi

		Q

		echo "-------------------------------------------------------------------------------"
	done 3<$1

	echo
	echo
	echo
	echo "Results: $Y passed, $N failed, score $(float "$Y*100/($Y+$N)")"
	echo
	echo "List of failed tests:"
	for((Y=0; Y<N; Y++)); do
		echo "${FAIL[$Y]}"
	done | nl
}

ch_tests $1
