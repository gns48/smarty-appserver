100 structure
    format json
    match @0.data
    @phones  = $phone_number
    @pass = $password_hash
    done 101
    error 900
    endstate

101 regex
    match @100.phones
    regex "([0-9]+)-([0-9]+)-([0-9]+)"
    done 801
    error 802
    @p1 = $1
    @p2 = $2
    @p3 = $3
    @p4 = '000'
#    @phone = "@101.p1@101.p2@101.p3"
    @phone = "$1$2$3"
    endstate

801 goto
    @800.status = @101.phone
    done 800
    endstate

802 goto
    @800.status = '0'    
    done 800
    endstate 

800 end
    data "{"state": "@status"}"
    data "@101.p1 @101.p2 @101.p3"
    data "flag: @101.p4"
    endstate








