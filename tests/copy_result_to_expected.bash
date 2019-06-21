for f in `find -name "result"`
do 
    echo $f
    cp $f ${f/result/expected}
done

for f in `find -name "*.out"`
do 
    echo $f
    cp $f ${f/out/expected}
done

for f in `find -name "result.*"`
do
    echo $f
    cp $f ${f/result/expected}
done
