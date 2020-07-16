#!/bin/sh
# $1: language list
# $2: path to index the exist facetype file
# $3: optional, the scope of font files, the final output must be in this scope
#langlist="$1 all unknown none"
langlist="$1 all" # 语言列表
fontlist=""   # 根据语言获取的字库列表
existlist=""  # 最终结果列表：在fontlist基础上添加文件是否存在和是否在限定列表中的过滤
path=$ANDROID_BUILD_TOP/$2  # Android.mk用到的路径
cfgfile=$ANDROID_BUILD_TOP/device/rda/common/lang_fonts.cfg # Config file path :lang_font.cfg

# 在配置文件的语言区域检索符合该语言的字库保存到fontforlang中, $1: the language to index
# 特别注意，如果是通过Makefile调用该脚本，不能用function定义函数，否则会出错
getFontForLang() {
    allforlang=`cat $cfgfile | grep -v "^#" | grep $1 `
    #echo "allforlang=<$allforlang>"
    #linenumber="`cat $cfgfile | grep -v "^#" | grep $1 | wc -l`"
    fontforlang=""
    #for (( i = 1; i <= $linenumber; i++ )) #是正常shell语法，但是Makefiile语法无法通过
    #while [ $linenumber -gt 0  ]
    for word in $allforlang
    do
        #if [[ $word =~ "." ]] #Android.mk中不合法
        fontnumberinword=`echo $word | grep "\."| wc -l`
        if [ $fontnumberinword = 1 ]
        then
            # 获取搜索结果的每一行
            line="`cat $cfgfile | grep -v "^#" | grep $word `"
            # echo "line=<$line>"
            # 删除最后一个空格及空格以后的字符，即去掉ttf，搜索语言定义部分
            langforline=${line% *} #获取语言部分:针对一个字库对应多个语言的情况
            if [ -n "`echo $langforline | grep $1`" ] # 如果搜索结果不为空
            then
                # 删除最后一个空格及空格之前的字符，即去掉ttf之前的语言部 分
                fontforline=${line##* } # 截取字库部分
                fontforlang=$fontforlang"$fontforline "
            fi
            #linenumber=$[ $linenumber - 1 ] # Android.mk语法无法通过
        else
            continue
        fi
    done
    echo $fontforlang
}

# 遍历输入语言列表，并将语言列表在lang_font.cfg中的字库查询结果保存到fontlist中
for locale in $langlist  #以空格作为分隔符遍历字符串
do
    result=`getFontForLang $locale`
    #echo "accurate result=<<<<$result>>>>>"
    if [ -n "$result" ] # 如果zh_CN的字库不为空，则直接追加zh_CN结果,不再追加zh的结果
    then
        fontlist=$fontlist"$result "
    else  # 如果为空，则搜索 zh的字库,如果不为空，则直接追加zh的结果
        #lang=${locale:0:2} # Android.mk不支持此语法
        lang=${locale%_*}
        result=`getFontForLang $lang`
        #echo "fuzzy result=<<<<$result>>>>>"
        if [ -n "$result" ]
        then
            fontlist=$fontlist"$result "
        fi
    fi
done
#echo "langlist=$langlist"
#echo "fontlist=$fontlist"

# 如果符合语言要求的字库不为空，则检索出在当前路径下存在的字库
if [ -n "$fontlist" ]
then
    for font in $fontlist
    do
    if [ -n "`find $path -name $font`" ]; then  # 判断文件是否存在
        # 如果输入参数有字库列表范围，则在以上结果中搜索列表范围中字库结果,保存在existlist中
        if [ $# -eq 3 ]
        then
            candidate=$3
            # 判断font是否是candidate的子字符串
            # if [[ $candidate =~ $font ]] #Android.mk中语法不合法
            existword=`echo $candidate | grep $font| wc -l`
            if [ $existword = 1 ]
            then
                existlist=$existlist"$font "
            # else
                # echo "$font: Not in Candidate"
            fi
        else
            existlist=$existlist"$font "
        fi
    # else
        # echo "$font : does NOT exist"
    fi
    done
fi
echo "$existlist"
