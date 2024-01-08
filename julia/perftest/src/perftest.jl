module perftest

greet() = print("Hello World!")

function getkey(i::Integer)::String
    prefix = "hello"
    prefix * string(i)
end

const numcalc = 10
const maxiter = 1000
const numtask = 8

function calc(cache::Dict{String, String})
    for i in 1:numcalc
        key = getkey(i)
        if haskey(cache, key)
            cache[key] = string(parse(Int64, cache[key]) + 1)
        else
            cache[key] = "1"
        end
        
    end
end

global tmpstream::Union{Nothing,IOStream}=nothing
global iodata::Vector{UInt8}=Vector{UInt8}(undef,1)

function fakeIO(sleepus::Int64)
    sleepus ≤ 0 && return 
    global tmpstream
    if isnothing(tmpstream)
        tmpstream =  open("/dev/zero", "r")
    end
    endtime = time_ns() + sleepus*1000
    while time_ns() ≤ endtime
        read!(tmpstream, iodata)
    end
end

Keyarr = ["Hello" * string(i) for i in 1:numcalc]
valarr = Dict(string(i) => string(i+1) for i in 0:maxiter*2)

function calc1(cache::Dict{String, String})
    for i in 1:numcalc
        key = keyarr[i]
        cache[key] = valarr[get(cache, key, "0")]
    end
end

function checkwork(cache::Dict{String, String}, checkval::Integer=maxiter*2)::Nothing
    strcheck = string(checkval)
    for i in 1:numcalc
        str = cache[getkey(i)]
        if str != strcheck
            throw(DomainError(str, "Expected $strcheck got $str"))
        end
    end
end

end # module perftest

function cachecalc(iotime::Int64)
    cache = Dict{String, String}()
    for i in 1:maxiter
        calc(cache)
        fakeIO(iotime)
        calc(cache)
    end
    checkwork(cache)
end

function cachecalc1(iotime::Int64)
    cache = Dict{String, String}()
    for i in 1:maxiter
        calc1(cache)
        fakeIO(iotime)
        calc1(cache)
    end
    checkwork(cache)
end

function cachecalc_mutex(iotime::Int64, lck = ReentrantLock())
    cache = Dict{String, String}()
    for i in 1:maxiter
        lock(lck) do
            calc(cache)
            fakeIO(iotime)
            calc(cache)
        end
    end
    checkwork(cache)
end

function cachecalc_threaded(iotime::Int64)
    cache = Dict{String, String}()
    lck = ReentrantLock()
    Threads.@threads for i in 1:maxiter
        lock(lck) do
            calc(cache)
        end
        fakeIO(iotime)
        lock(lck) do
            calc(cache)
        end
    end
    checkwork(cache)
end

function cachecalc_channel()
    calcch = Channel{Boolean}(100, spawn=true) do ch
        
    end
end
