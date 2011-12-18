tile_temp = _.template('
<div
  class="tile tile_<%=data.x%>_<%=data.y%> tiletype_<%=data.type%>"
  data-robot=\'<%=JSON.stringify(data)%>\'>
</div>')


mapsize = 10


resources =
  shovel: 1
  seed: 0
  energy: 0
  solar: 1

cursor =
  pointer:0
  shovel:1
  seed:2
  solar:3

obs =
  none: 0
  stone:1
  oil:2
  trash:3

action =
  shovel:0

map = []


cc = cursor.pointer

mapgen = () ->
  return _(_.range(mapsize)).map () ->
    return _(_.range(mapsize)).map () ->
      return {
        type:Math.floor(Math.random()*3 + 1)
      }

map.push mapgen()

draw = () ->
  for x in [0..mapsize-1]
    for y in [0..mapsize-1]
       $("#map").append tile_temp(
         data: {x:x,y:y,type:map[0][x][y].type})


update = () ->
  for x in [0..mapsize-1]
    for y in [0..mapsize-1]
       $(".tile_" + x + "_" + y)
$ () ->
  draw()


  $('.tool').live 'click', ()->
    cc = cursor[$(this).data('robot')]
    console.log $(this).data('robot')
    console.log cc

  $('.tile').live 'click', ()->
    console.log($(this))
    data = $(this).data('robot')
    if resources.shovel > 0
      map[0][data.x][data.y].do = action.shovel
      resources.shovel -= 1
      map[0][data.x][data.y].doprogress = map[0][data.x][data.y].doprogress || 100

  setInterval (()->
    for x in [0..mapsize-1]
      for y in [0..mapsize-1]
        tile = map[0][x][y]

        if tile.doprogress <= 0
          if tile.do == action.shovel
            tile.do = false
            tile.type = obs.none
            resources.seed += 1
            resources.shovel += 1
        else
          tile.doprogress -= 5

        $('.tile_'+x+'_'+y).removeClass()
          .addClass('tile tile_'+x+'_'+y)
          .addClass('tiletype_'+tile.type)
          .addClass('tileprogress_'+tile.doprogress)

    $("#seed").html("seeds: " + resources.seed)
    $("#shovel").html("shovels: " + resources.shovel)

  ),100
