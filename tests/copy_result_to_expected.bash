for f in `find -name "result"`
do 

    if [ -f "${f/result/expected}" ]
    then
        echo $f
        cp $f ${f/result/expected}
    fi
done

for f in `find -name "*.out"`
do 
    echo $f
    if [ -f "${f/out/expected}" ]
    then
        cp $f ${f/out/expected}
    fi
done

for f in `find -name "result.*"`
do
    echo $f
    if [ -f "${f/result/expected}" ]
    then
        cp $f ${f/result/expected}
    fi
done
