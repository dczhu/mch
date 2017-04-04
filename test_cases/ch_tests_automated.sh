ch_tests()
{
	pad()
	{
		if [ "$1" -lt 10 ]; then
			echo "00""$1"
		elif [ "$1" -lt 100 ]; then
			echo "0""$1"
		fi
	}

	[ ! -f "$1" ] && { echo "File \"$1\" doesn't exist"! ; echo "Usage: $FUNCNAME \$test_case_file"; return 1; }

	echo "-------------------------------------------------------------------------------"

	local LINE LINEP idx=0 pidx

	# Use raw mode of the built-in read.
	while read -r LINE; do
		if [ "${LINE//[ 	]/}" == "" ] || [ "$(echo "${LINE}" | sed -e "s/^[ \t]*#//")" != "${LINE}" ]; then
			echo "    === $LINE"
			continue
		fi

		let idx++
		pidx=$(pad $idx)
		echo "$pidx === $LINE"

		eval "$LINE"

		# To test h, replace ' ' with | and substitute grep -P for h.
		LINEP="$LINE"
		LINEP="${LINEP//| h/| /bin/grep -P --color=always}"
		LINEP="${LINEP//| cxpgrep?/| /bin/grep -P --color=always }"
		LINEP="${LINEP//\' \'/|}"
		LINEP="${LINEP//\" \"/|}"
		if [ "$LINEP" != "$LINE" ]; then
			echo "    === $LINEP"

			eval "$LINEP"
		fi

		echo "-------------------------------------------------------------------------------"
	done <$1
}

ch_tests $1
