
100 regex
    match @0.sum
    regex "([0-9]+).([0-9]+)"
    done 150
    error 300
    @rub = $1
    @kop = $2
    endstate

150 structure
    format json
    match @0.data
    @val  = $state_code.a
    @text = $state_code.b
    done 200
    error 300
    endstate

200 end
    data "<p>data = @0.data</p>" 
    data "<p>sum = @0.sum</p>"
    data "<p>rub = @100.rub</p>"
    data "<p>kop = @100.kop</p>"
    data "<p>json state code @150.val</p>"  
    data "<p>json state text @150.text<p>"    
    endstate

300 end
    data 'Can not parse/match regex'
    endstate

