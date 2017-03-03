BEGIN {
	stderr = "/dev/stderr"
	ver = 1.01
	print "Ronsor UnOptimizing Assembler " ver " (C) 2016, 2017 Ronsor-OpenStar" > stderr
	print "This is FREE SOFTWARE under the Ronsor Public License 4.0" > stderr
	print "NO WARRANTY." > stderr
	opcodes["settlen"] = 0x11
	opcodes["inc"] = 0x10
	opcodes["dup"] = 3
	opcodes["outc"] = 2
	opcodes["jmp"] = 6
	opcodes["ret"] = 7
	opcodes["call"] = 8
	opcodes["stw"] = 5
	opcodes["ldw"] = 4
	opcodes["int"] = 9
	opcodes["pushivt"] = 10
	opcodes["swap"] = 11
	opcodes["bei"] = 12
	opcodes["eni"] = 13
	opcodes["add"] = 0x15;
	opcodes["sub"] = 0x16;
	opcodes["mul"] = 0x17;
	opcodes["div"] = 0x18;
	opcodes["not"] = 0x19;
	opcodes["eq"] = 0x20;
	opcodes["gt"] = 0x21;
	opcodes["lt"] = 0x22;
	opcodes["jme"] = 0x25;
	total = 1;
	pc = 0;
}
function parse_csv(string,csv,sep,quote,escape,newline,trim, fields,pos,strtrim) {
    # Make sure there is something to parse.
    if (length(string) == 0) return 0;
    string = sep string; # The code below assumes ,FIELD.
    fields = 1; # The number of fields found thus far.
    while (length(string) > 0) {
        # Remove spaces after the separator if requested.
        if (trim && substr(string, 2, 1) == " ") {
            if (length(string) == 1) return fields;
            string = substr(string, 2);
            continue;
        }
        strtrim = 0; # Used to trim quotes off strings.
        # Handle a quoted field.
        if (substr(string, 2, 1) == quote) {
            pos = 2;
            do {
                pos++
                if (pos != length(string) &&
                    substr(string, pos, 1) == escape &&
                    (substr(string, pos + 1, 1) == quote ||
                     substr(string, pos + 1, 1) == escape)) {
                    # Remove escaped quote characters.
                    string = substr(string, 1, pos - 1) substr(string, pos + 1);
                } else if (substr(string, pos, 1) == quote) {
                    # Found the end of the string.
                    strtrim = 1;
                } else if (newline && pos >= length(string)) {
                    # Handle embedded newlines if requested.
                    if (getline == -1) {
                        csverr = "Unable to read the next line.";
                        return -1;
                    }
                    string = string newline $0;
                }
            } while (pos < length(string) && strtrim == 0)
            if (strtrim == 0) {
                csverr = "Missing end quote.";
                return -2;
            }
        } else {
            # Handle an empty field.
            if (length(string) == 1 || substr(string, 2, 1) == sep) {
                csv[fields] = "";
                fields++;
                if (length(string) == 1)
                    return fields;
                string = substr(string, 2);
                continue;
            }
            # Search for a separator.
            pos = index(substr(string, 2), sep);
            # If there is no separator the rest of the string is a field.
            if (pos == 0) {
                csv[fields] = substr(string, 2);
                fields++;
                return fields;
            }
        }
        # Remove spaces after the separator if requested.
        if (trim && pos != length(string) && substr(string, pos + strtrim, 1) == " ") {
            trim = strtrim
            # Count the number fo spaces found.
            while (pos < length(string) && substr(string, pos + trim, 1) == " ") {
                trim++
            }
            # Remove them from the string.
            string = substr(string, 1, pos + strtrim - 1) substr(string,  pos + trim);
            # Adjust pos with the trimmed spaces if a quotes string was not found.
            if (!strtrim) {
                pos -= trim;
            }
        }
        # Make sure we are at the end of the string or there is a separator.
        if ((pos != length(string) && substr(string, pos + 1, 1) != sep)) {
            csverr = "Missing separator.";
            return -3;
        }
        # Gather the field.
        csv[fields] = substr(string, 2 + strtrim, pos - (1 + strtrim * 2));
        fields++;
        # Remove the field from the string for the next pass.
        string = substr(string, pos + 1);
    }
    return fields;
}
function ltrim(s) { sub(/^[ \t\r\n]+/, "", s); return s }
function rtrim(s) { sub(/[ \t\r\n]+$/, "", s); return s }
function trim(s) { return rtrim(ltrim(s)); }
{
	$0 = trim($0)
	nfields = parse_csv($0,fields," ","\"","\\",1,1)
	
	fields[1] = trim(fields[1]);
}
function ord(str,  i) {
	if ( _ord["A"] == "" ) { for(i=0;i<256;i++) _ord[sprintf("%c", i)] = i }
	return _ord[str]
}
function tohex(str, digit, x, number) {
	for (x=1;x<=length($1);x++){
digit=index(str,substr($1,x,1));
number=number*16+digit}
	return number
}
function getval(str) {
	if ( substr(str, 1, 1) == "@" ) {
		if ( symbols[substr(str,2,length(str)-1)] != "" ) {
		return symbols[substr( str, 2, length(str)-1)]
		} else {
		print "Symbol " str " not found." > stderr
		exit(1)
		}
	}
	if ( substr(str, 1, 1) == "'" ) {
		return ord(substr(str, 2, 1))
	}
	if ( substr(str, 1, 2) == "0x" ) {
		return tohex(substr(str, 3, length(str)-2))
	}
	return str + 0
}
fields[1] == ":" {
	symbols[fields[2]] = pc
	next
}
fields[1] == "push" { pc+= 2 }
fields[1] == "string" {

	opc += length(fields[2])
}
fields[1] == "dw" {
	opc += 1
}
{
	lines[total] = $0
	total++
}
{ 
if ( fields[1] in opcodes) {
	pc++
}
}
END {

x = 0
for ( x = 1; x < total; x++ ) {
nfields = parse_csv(lines[x],fields, " ", "\"", "\\", 1, 1);
#print fields[1] ": " fields[2]
if ( fields[1] == "push" ) {
	printf("1 %x ", getval(fields[2]));
	continue
}
if ( fields[1] == "dw" ) {
	printf("%x ", getval(fields[2]));
	continue
}
if ( fields[1] == "string" ) {
	for ( i=1;i<=length(fields[2]);i++ ) {
		printf("%x ", ord(substr(fields[2],i,1)))
	}
	continue
}
if ( fields[1] in opcodes ) {
	printf("%x ", opcodes[fields[1]]);
}
}
for ( n in symbols ) {
	print sprintf("Symbol: %s = %x", n, symbols[n]) > stderr
}
}

