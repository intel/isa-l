#/bin/sh

# Filter out unnecessary options added by automake

while [ -n "$*" ]; do
    case "$1" in
	-o )
	    # Supported options with arg
	    options="$options $1 $2"
	    shift
	    object="$1"
	    shift
	    ;;
	-f | -D )
	    # Supported options with arg
	    options="$options $1 $2"
	    shift
	    shift
	    ;;
	-I | -i )
	    options="$options $1 $2/"
	    shift
	    shift
	    ;;
	--prefix* )
	    # Supported options without arg
	    options="$options $1"
	    shift
	    ;;
	-I* | -i* )
	    options="$options $1/"
	    shift
	    ;;
	-D* ) # For defines we need to remove spaces
	    case "$1" in
		*' '* ) ;;
		*) options="$options $1" ;;
	    esac
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

nasm $options $args
$CET_LD -r -z ibt -z shstk -o $object.tmp $object
mv $object.tmp $object
