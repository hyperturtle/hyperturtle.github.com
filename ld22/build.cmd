call cd assets
call lessc style.less style.css
call coffee -c script.coffee
call uglifyjs -o script.min.js script.js
