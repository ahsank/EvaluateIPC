module perftest

greet() = print("Hello World!")

function getkey(i::Integer)::String
    prefix = "hello"
    prefix * string(i)
end

const numcalc = 10
const maxiter = 1000
const numtask = 8
const start_val = 10000

function calc(cache::Dict{String, String})
    for i in 1:numcalc
        key = getkey(i)
        if haskey(cache, key)
            cache[key] = string(parse(Int64, cache[key]) + 1)
        else
            cache[key] = string(start_val+1)
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

# Pre-allocated keys and value mapping to avoid parse/string overhead in the optimized version
const Keyarr = ["hello" * string(i) for i in 1:numcalc]
const ValMap = Dict(string(i) => string(i + 1) for i in 0:maxiter * 2 + start_val)

function calc_optimized(cache::Dict{String, String})
    for i in 1:numcalc
        key = Keyarr[i]
        # Avoid parse/string by using the pre-computed ValMap
        current_val = get(cache, key, string(start_val))
        cache[key] = ValMap[current_val]
    end
end

function checkwork(cache::Dict{String, String}, checkval::Integer=start_val + maxiter*2)::Nothing
    strcheck = string(checkval)
    for i in 1:numcalc
        str = cache[getkey(i)]
        if str != strcheck
            throw(DomainError(str, "Expected $strcheck got $str"))
        end
    end
end


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
        for i ∈ 1::maxiter
            put!(ch, true)
        end
    end
    
end

function cachecalc_spawned(iotime::Int64)
    cache = Dict{String, String}()
    lck = ReentrantLock()
    
    # Use @sync and @spawn for dynamic scheduling
    @sync for i in 1:maxiter
        Threads.@spawn begin
            lock(lck) do
                calc(cache)
            end
            fakeIO(iotime)
            lock(lck) do
                calc(cache)
            end
        end
    end
    checkwork(cache)
end

function cachecalc_optimized_spawned(iotime::Int64)
    cache = Dict{String, String}()
    lck = ReentrantLock()
    
    # Use @sync and @spawn for dynamic scheduling
    @sync for i in 1:maxiter
        Threads.@spawn begin
            lock(lck) do
                calc_optimized(cache)
            end
            fakeIO(iotime)
            lock(lck) do
                calc_optimized(cache)
            end
        end
    end
    checkwork(cache)
end


end # module perftest
