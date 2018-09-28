100 regex 
    regex "([a-z][a-z0-9]+)\s*=\s*([0-9]+)"
    done 110
    error 300
    name = $1
    value = $2
    endstate

110 match
    match @100.value
    case "[a-z][a-z]+" 150
    case "[0-9]+" 151
    else 152
    error 300
    endstate

150 file
    file '/tmp/match.sl.output'
    done 200
    error 250
    data "Matched alphabetic: @100.value\n"
    endstate 

151 file
    file '/tmp/match.sl.output'
    done 200
    error 250
    data "Matched numeric: @100.value\n"
    endstate 

152 file
    file '/tmp/match.sl.output'
    done 200
    error 250
    data 'Matched none: @100.value\n'
    endstate 

200 end
    endstate

250 end
    data "Can not write file"
    endstate

300 end
    data 'Can not parse/match regex'
    endstate
