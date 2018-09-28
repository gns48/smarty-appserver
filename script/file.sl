100 regex
    match @0.function 
    regex "([a-z][a-z0-9]+)\s*=\s*([0-9]+)"
    done 110
    error 300
    @name = $1
    @value = $2
    endstate

110 file
    file '/tmp/file.sl.output'
    done 200
    error 250
    data "name = @100.name\n"
    data "value = @100.value\n"
    endstate

200 end
    data "name = @100.name\n"
    data "value = @100.value\n"
    endstate

250 end
    data "Can not write file at state 110\n"
    endstate

300 end
    data 'Can not parse/match regex'
    endstate
