#/bin/sh

# Filter out unnecessary options added by automake

while [ -n "$*" ]; do
    case "$1" in
	-f | -o | -I | -i | -D )
	    # Supported options with arg
	    options="$options $1 $2"
	    shift
	    shift
	    ;;
	-I* | -i* | --prefix* )
	    # Supported options without arg
	    options="$options $1"
	    shift
	    ;;
	#-blah )
	# Unsupported options with args - none known
	-* )
	    # Unsupported options with no args
	    shift
	    ;;
	* )
	    args="$args $1"
	    shift
	    ;;
    esac
done

yasm $options $args
