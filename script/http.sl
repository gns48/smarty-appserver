100 http
    url "http://localhost/smarty.cgi"
    parameters "?function=regex\&id_smarty=901222\&sum=25.00"
    parameters "\&data={\"state_code\":1011010102}"
    outputvar @httpresult
    done 200
    error 300
    endstate

200 end
    data "@100.httpresult"
    endstate

300 end
    data 'Can not complete resuest'
    endstate
