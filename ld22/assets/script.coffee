tile_temp = _.template('
<div
  class="tile tile_<%=data.x%>_<%=data.y%> tiletype_<%=data.type%>"
  data-robot=\'<%=JSON.stringify(data)%>\'>
</div>')


mapsize = 10

cursor =
  drill: 0
  laser: 1
  grass: 2
  plant: 3
  tree:  4
  forest:5
  cloud: 6

tt =
  none:  0
  ice:   1
  water: 2
  trash: 3
  grass:  100
  plant:  101
  tree:   102
  forest: 103

tts = _(tt).chain().values().map( (x) -> 'tiletype_' + x ).value().join(' ')
cursors = _(cursor).chain().values().map( (x) -> 'cursor_' + x ).value().join(' ')

cc = 0

map = []

clouds = []

mapgen = () ->
  return _(_.range(mapsize)).map (x) ->
    return _(_.range(mapsize)).map (y) ->
      return {
        type: Math.floor(Math.random()*4)
        x:x
        y:y
      }


draw = () ->
  for x in [0..mapsize-1]
    for y in [0..mapsize-1]
       map[0][x][y].tile = $(tile_temp(
         data: {x:x,y:y,type:map[0][x][y].type})
       ).appendTo("#map")

getnewcursor = () ->
  cc = Math.floor(Math.random()*3)
  $("#cursor").removeClass(cursors).addClass('cursor_'+cc)

getsurrounding = (x,y, type) ->
  return _([[x+1,y],[x-1,y],[x,y+1],[x,y-1]])
    .chain()
    .filter( (x) -> x[0] >= 0 and x[0] < mapsize )
    .filter( (x) -> x[1] >= 0 and x[1] < mapsize )
    .filter( (x) -> type(map[0][x[0]][x[1]].type) )
    #.map( (x) -> map[0][x[0]][x[1]] )
    .value()

getsim = (x,y,type) ->
  points = _.union([[x,y]],getsurrounding(x,y, type))
  old_length = 0
  while old_length != points.length
    old_length = points.length
    for point in points
      points = _.union(points, getsurrounding(point[0], point[1], type))
      points = _.unique points, false, (x) -> x[0] + ',' + x[1]

  return points

  
updatetile = (tile) ->
  tile.tile.removeClass(tts)
    .addClass('tiletype_'+tile.type)

checktile = (tile) ->
  switch tile.type
    when tt.grass, tt.plant, tt.tree, tt.forest
      blob = getsim(tile.x,tile.y,(x) -> x == tile.type or x == tt.water)
    else
      return
      blob = getsim(tile.x,tile.y,(x) -> x == tile.type)

  switch tile.type
    when tt.grass, tt.plant, tt.tree, tt.forest, tt.water
      if blob.length >= 3
        type = tile.type

        for ti in blob
          map[0][ti[0]][ti[1]].type = tt.none
          updatetile(map[0][ti[0]][ti[1]])

        type = Math.min(tt.tree, type + Math.floor(blob.length/3) - 1)
        console.log type

        switch type
          when tt.tree
            tile.type = tt.forest
          when tt.plant
            tile.type = tt.tree
          when tt.grass
            tile.type = tt.plant
        updatetile(tile)
        return true
  return false

getstats = () ->
  stats = {}
  for x in [0..mapsize-1]
    for y in [0..mapsize-1]
      stats[map[0][x][y].type] = stats[map[0][x][y].type] || 0
      stats[map[0][x][y].type] += 1

  return stats

$ () ->
  map.push mapgen()
  getnewcursor()
  draw()

  $('.tile').live 'click', () ->
    data = $(this).data('robot')
    tile = map[0][data.x][data.y]

    consumed = true

    switch cc
      when cursor.grass
        switch tile.type
          when tt.none
            tile.type = tt.grass
          else
            consumed = false
      when cursor.drill
        switch tile.type
          when tt.trash
            tile.type = tt.none
          when tt.grass, tt.plant, tt.tree, tt.none
            tile.type = tt.none
          else
            consumed = false
      when cursor.laser
        switch tile.type
          when tt.ice
            tile.type = tt.water
          when tt.water, tt.none
            tile.type = tt.none
      else
        consumed = false
        
    if consumed
      updatetile(tile)

      while true
        break unless checktile(tile)
      getnewcursor()

    console.log getstats()
    


