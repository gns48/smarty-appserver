; http://localhost/smarty.cgi?function=query&min=9&max=18
100 query
    db 'testdb'
    query "select lMount, lName from lenses where lFocal = @0.min and lFocalMax = @0.max"
    done 200
    error 300
    @mount = $0
    @name = $1
    endstate

200 end
    data "<p>mount = @100.mount</p>"
    data "<p>name = @100.name</p>"
    endstate

300 end
    data 'Can not query the database'
    endstate
