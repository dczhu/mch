h()
{
	_usage() { 
		echo "usage: YOUR_COMMAND | h [-i] args...
	-i : ignore case"
	}

	local _OPTS

	# Detect pipe or tty
	if test -t 0; then 
		_usage
		return
	fi

	# Manage flags
	while getopts ":i" opt; do
		case $opt in 
		i) _OPTS+=" -v IGNORECASE=1 " ;;
		\?) _usage; return ;;
		esac
	done

	shift $(($OPTIND - 1))

	local _P _I=1
	local -a _A

	# Process some special chars (', ", `, <, >, \) for AWK's needs.
	for _P in "$@"; do
		# Convert (even number of \)\' to (even number of \)'
		_P="$(echo "$_P" | sed -r -e "s/(^(\\\\\\\\)*)\\\\'/\1'/" -e "s/([^\\](\\\\\\\\)*)\\\\'/\1'/g")"
		_P="${_P//\'/\\047}"
		# Convert (even number of \)\" to (even number of \)"
		_P="$(echo "$_P" | sed -r -e 's/(^(\\\\)*)\\"/\1"/' -e 's/([^\\](\\\\)*)\\"/\1"/g')"
		_P="${_P//\"/\\042}"
		# Convert (even number of \)\` to (even number of \)`
		_P="$(echo "$_P" | sed -r -e 's/(^(\\\\)*)\\`/\1`/' -e 's/([^\\](\\\\)*)\\`/\1`/g')"
		_P="${_P//\`/\\140}"
		# Convert (even number of \)\< to (even number of \)<
		_P="$(echo "$_P" | sed -r -e 's/(^(\\\\)*)\\</\1</' -e 's/([^\\](\\\\)*)\\</\1</g')"
		_P="${_P//\</\\074}"
		# Convert (even number of \)\> to (even number of \)>
		_P="$(echo "$_P" | sed -r -e 's/(^(\\\\)*)\\>/\1>/' -e 's/([^\\](\\\\)*)\\>/\1>/g')"
		_P="${_P//\>/\\076}"
		# Convert (even number of \)\y to (even number of \'s)y
		# This is because grep -P takes \y as y (no special meaning), whereas AWK uses \y for empty string at the edge of a word.
		_P="$(echo "$_P" | sed -r -e 's/(^(\\\\)*)\\y/\1y/' -e 's/([^\\](\\\\)*)\\y/\1y/g')"
		# Convert (even number of \)\b to (even number of \)\y
		# This is because grep -P takes \b for empty string at the edge of a word, whereas AWK uses \y.
		_P="$(echo "$_P" | sed -r -e 's/(^(\\\\)*)\\b/\1\\y/' -e 's/([^\\](\\\\)*)\\b/\1\\y/g')"
		# Replace \ to \134, but recover other \nnn instances that have been converted to \134nnn. This fixes something like:
		# echo "tastb\\'c" | h "b\\\'c"
		_P="$(echo "$_P" | sed -r -e 's/\\/\\134/g' -e 's/\\134([0-7]{3})/\\\1/g')"
		_A[$_I]="${_P}"
		let _I++
	done

	cat - | awk --re-interval $_OPTS '
	func pad(num)
	{
		if (num < 10) {
			num = "0000"num
		} else if (num < 100) {
			num = "000"num
		} else if (num < 1000) {
			num = "00"num
		} else if (num < 10000) {
			num = "0"num
		}

		return num
	}

	func get_start_end(pat, idx,     from, tmppat)
	{
		if (pat ~/^[*?]/) {
			print "h: nothing to repeat" > "/dev/stderr"
			# Exit 2 to be compatible with "grep -P".
			exit 2
		}

		if (gensub(/(\\\\)*$/, "", "g", pat) ~/\\$/) {
			print "h: \\ at end of pattern" > "/dev/stderr"
			# Exit 2 to be compatible with "grep -P".
			exit 2
		}

		if (pat == "")
			return -1

		from = 1
		while (match(substr($0, from), pat) > 0) {
			if (RLENGTH == 0) {
				tmppat = pat
				tmppat = gensub(/([^\\])\*/, "\\1", "g", tmppat)
				tmppat = gensub(/(\\\\)\*/, "\\1", "g", tmppat)

				tmppat = gensub(/([^\\])\?/, "\\1", "g", tmppat)
				tmppat = gensub(/(\\\\)\?/, "\\1", "g", tmppat)

				# Change r{0,x} to r{1,x}
				tmppat = gensub(/([^\\]\{)0(,)/, "\\11\\2", "g", tmppat)
				tmppat = gensub(/(\\\\\{)0(,)/, "\\11\\2", "g", tmppat)

				# Change a||b to a|b, and change a| to a, and change |a to a.
				tmppat = gensub(/([^\\](\\\\)*)\|\|+/, "\\1|", "g", tmppat)
				tmppat = gensub(/(^(\\\\)+)\|\|+/, "\\1|", "g", tmppat)

				tmppat = gensub(/([^\\](\\\\)*)\|+$/, "\\1", "g", tmppat)
				tmppat = gensub(/(^(\\\\)+)\|+$/, "\\1", "g", tmppat)

				tmppat = gensub(/^\|+/, "", "g", tmppat)
				if (tmppat == pat) {
					# The pattern is something like "|". Give up.
					break
				} else {
					pat = tmppat
					continue
				}
			}
			a[pad(RSTART + from - 1), "-1", idx] = ""
			a[pad(RSTART + from - 1 + RLENGTH), "+1", idx] = ""
			if (pat ~/^\^/) {
				# A pattern matching the start of a line can only match once!
				# The line-end matching does not have this issue as the matching
				# loop above has finished after this point.
				break
			}
			from += RSTART + RLENGTH - 1
		}
		return 0
	}

	func split_array_content(arr, num,     i, j, tmp)
	{
		for (i = 1; i <= num; i++) {
			split(arr[i], tmp, /\x1c/)
			for (j = 1; j <= 3; j++)
				arr[i, j] = tmp[j];
		}
	}

	func establish_color_points(arr, num,     i, gi, sum, color, start)
	{
		gi = 1
		sum = 0
		color = -1
		for (i = 1; i <= num; i++) {
			if (arr[i, 2] == "-1") {
				# Start point of a pattern
				sum += -1
				if (sum < -1)
					color = NUMCOLORS + 1
				else if (sum == -1)
					start = arr[i, 1]
			} else {
				# End point of a pattern
				sum += 1
				if (sum == 0) {
					if (color > NUMCOLORS) {
						g[gi, 1] = start
						g[gi, 2] = color
						gi++
						g[gi, 1] = arr[i, 1]
						g[gi, 2] = 0
						gi++
						color = -1
					} else {
						g[gi, 1] = start
						g[gi, 2] = arr[i, 3]
						gi++
						g[gi, 1] = arr[i, 1]
						g[gi, 2] = 0
						gi++
					}
				}
			}
		}

		return gi - 1
	}

	func mix_in_colors(num,     i, start, str)
	{
		start = 1
		str = ""
		for (i = 1; i <= num; i++) {
			str = str""substr($0, start, g[i, 1] - start)""c[g[i, 2]]
			start = g[i, 1]
		}
		str = str""substr($0, start)

		return str
	}

	BEGIN {
		no_color = "\x1b[0m"

		red = "\x1b[1;4;31m"
		green = "\x1b[1;4;32m"
		blue = "\x1b[1;4;34m"
		purple = "\x1b[1;4;35m"
		cyan = "\x1b[1;4;36m"
		olive = "\x1b[1;4;38;5;100m"
		pink = "\x1b[1;4;38;5;201m"
		gold = "\x1b[1;4;38;5;172m"

		dark_yellow_bg = "\x1b[1;4;43m"

		# Or NUMPATTERNS
		NUMCOLORS = 8

		c[0] = no_color

		c[1] = red
		c[2] = green
		c[3] = blue
		c[4] = purple
		c[5] = cyan
		c[6] = olive
		c[7] = pink
		c[8] = gold

		c[9] = dark_yellow_bg

		p[1] = "'"${_A[1]}"'"
		p[2] = "'"${_A[2]}"'"
		p[3] = "'"${_A[3]}"'"
		p[4] = "'"${_A[4]}"'"
		p[5] = "'"${_A[5]}"'"
		p[6] = "'"${_A[6]}"'"
		p[7] = "'"${_A[7]}"'"
		p[8] = "'"${_A[8]}"'"

		# p[] contains all the patterns to be highlighted.
	}

	{
		delete a
		for (i = 1; i <= NUMCOLORS; i++)
			get_start_end(p[i], i)

		# At this point: a[padded position number of pattern start/end points, indicator of start/end, color index]

		n = asorti(a)

		# At this point: a[1~x] each element contains the index string of the former a[] (the position numbers in current elements are ascending).

		split_array_content(a, n)

		# At this point: a[1~x, 1~3] --- 1 - padded position number of pattern start/end points; 2 - indicator of start/end; 3 - color index.

		delete g
		n = establish_color_points(a, n)

		# At this point: g[1~y, 1~2] --- 1 - padded position number of points where a color escape sequence should be placed then followed by the original content of the line at that point (overlapped patterns will be enclosed by a special color in the background, otherwise the color associated with the pattern); 2 - the color index (note that color 0 is no-color which is used to stop a pattern highlighting).

		print mix_in_colors(n)

		# According to g[], the color sequences have been mixed with the original content of the line. The new string is printed.
	}
	'
}
