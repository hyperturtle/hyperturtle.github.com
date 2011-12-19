tile_temp = _.template('
<div
  class="tile tile_<%=data.x%>_<%=data.y%> tiletype_<%=data.type%>"
  data-robot=\'<%=JSON.stringify(data)%>\'>
</div>')

cloud_temp = _.template('
<div
  class="cloud cloudid_<%=data.id%> cloudtype_<%=data.type%>"
  style="top:<%=data.x*50%>px; left:<%=data.y*50%>px"
  data-robot=\'<%=JSON.stringify(data)%>\'>
</div>')


res_temp = _.template('
<b>Storage</b><br/>
<a href="" class="res" data-robot="2">grass:</a> infin<br/>
<a href="" class="res" data-robot="0">drill:</a> infin<br/>
<a href="" class="res" data-robot="1">laser:</a>  <%=laser%><br/>
<a href="" class="res" data-robot="3">plant:</a>  <%=plant%>  <br/>
<a href="" class="res" data-robot="4">tree:</a>   <%=tree%>   <br/>
<a href="" class="res" data-robot="5">forest:</a> <%=forest%> <br/>
<a href="" class="res" data-robot="6">cloud:</a>  <%=cloud%>  <br/>
')

stats_temp = _.template('
<b>Progres</b><br/>
Water: <%=water%> <br/>
Plants:  <%=plant%>  <br/>
')

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
  acid:  4
  grass:  100
  plant:  101
  tree:   102
  forest: 103

ct =
  none:  -1
  acid:   0
  water:  1

resources =
  laser:0
  plant:0
  tree:0
  forest:0
  cloud:0

tts = _(tt).chain().values().map( (x) -> 'tiletype_' + x ).value().join(' ')
cursors = _(cursor).chain().values().map( (x) -> 'cursor_' + x ).value().join(' ')

cc = 0

map = []

clouds = []

unlocked = 0


mapgen = (diff = 0) ->
  map = []
  clouds = []
  map.push _(_.range(mapsize)).map (x) ->
    return _(_.range(mapsize)).map (y) ->
      type = tt.none
      switch diff
        when 0
          type = Math.floor(Math.random()*4)
        when 1
          type = Math.floor(Math.random()*5)
        when 2
          type = Math.floor(Math.random()*5)
          if type == tt.water
            type = tt.trash
        when 3
          type = Math.floor(Math.random()*5)
          if type == tt.water
            type = tt.trash
          if type == tt.water
            type = tt.trash
      return {
        type: type
        x:x
        y:y
      }

  cloudc = 0
  switch diff
    when 1
      cloudc = 10
    when 2
      cloudc = 20
    when 3
      cloudc = 40

  while clouds.length < cloudc
    addCloud(Math.floor(Math.random()*mapsize), Math.floor(Math.random()*mapsize), ct.acid)
draw = () ->
  for x in [0..mapsize-1]
    for y in [0..mapsize-1]
       map[0][x][y].tile = $(tile_temp(
         data: {x:x,y:y,type:map[0][x][y].type})
       ).appendTo("#map")


switchcursor = (newcc) ->
  cc = newcc
  $("#cursor").removeClass(cursors).addClass('cursor_'+cc)
  $("#info").removeClass().addClass('ct_'+cc)

getnewcursor = () ->
  cc = 2 #Math.floor(Math.random()*7)
  $("#cursor").removeClass(cursors).addClass('cursor_'+cc)
  $("#info").removeClass().addClass('ct_'+cc)

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
    when tt.grass, tt.plant, tt.tree
      blob = getsim(tile.x,tile.y,(x) -> x == tile.type or x == tt.water)
    else
      return
      blob = getsim(tile.x,tile.y,(x) -> x == tile.type)

  switch tile.type
    when tt.grass, tt.plant, tt.tree, tt.water
      if blob.length >= 3
        type = tile.type

        for ti in blob
          map[0][ti[0]][ti[1]].type = tt.none
          updatetile(map[0][ti[0]][ti[1]])

        type = Math.min(tt.tree, type + Math.floor(blob.length/3) - 1)
        #console.log type

        switch type
          when tt.tree
            tile.type = tt.forest
          when tt.plant
            tile.type = tt.tree
          when tt.grass
            tile.type = tt.plant
        updatetile(tile)
addCloud = (x,y, type) ->
  for cloud in clouds
    if cloud.x == x and cloud.y == y
      return false
  clouds.push {x:x, y:y, type:type}
  $("cloudid_" + clouds.length).show()

  $(cloud_temp(
    data: {id:clouds.length, type: if (type == ct.acid) then 'acid' else 'water'}
  )).appendTo("#map")

getstats = () ->
  stats = {}
  for x in [0..mapsize-1]
    for y in [0..mapsize-1]
      stats[map[0][x][y].type] = stats[map[0][x][y].type] || 0
      stats[map[0][x][y].type] += 1

  wclouds = 0
  for cloud in clouds
    if cloud.alive && cloud.type == ct.water
      wclouds += 1

  plant = 0
  plant += stats[tt.grass] || 0
  plant += stats[tt.plant]*2 || 0
  plant += stats[tt.tree]*4 || 0
  plant += stats[tt.forest]*8 || 0
  water = stats[tt.water] + wclouds * 5
  #console.log stats

  if plant >= 100 and water >= 50
    unlocked += 1
    $(".locked" + unlocked).removeClass("locked")


  return {
    plant: (plant || 0) + '/100'
    water: (water || 0) + '/50'
  }

addCloud = (x,y, type) ->
  for cloud in clouds
    if cloud.x == x and cloud.y == y
      if cloud.type == type
        return false
  id = clouds.length
  clouds.push {x:x, y:y, type:type, id:id, alive:true}

  $(cloud_temp(
    data: {id:id, x:x, y:y, type: if (type == ct.acid) then 'acid' else 'water'}
  )).appendTo("#map")

updateclouds = () ->

  #console.log clouds
  for cloud in clouds
    if not cloud.alive
      $('.cloudid_' + cloud.id).hide()
      continue
    x = cloud.x
    y = cloud.y
    newpos = _([[x,y], [x+1,y],[x-1,y],[x,y+1],[x,y-1]])
      .chain()
      .filter( (x) -> x[0] >= 0 and x[0] < mapsize )
      .filter( (x) -> x[1] >= 0 and x[1] < mapsize )
      .shuffle()
      .value()[0]
    cloud.x = newpos[0]
    cloud.y = newpos[1]


    #console.log cloud

    $('.cloudid_' + cloud.id).css
      top:cloud.x * 50
      left:cloud.y * 50

checkclouds = () ->
  for cloud in clouds
    tile = map[0][cloud.x][cloud.y]

    for cloud2 in clouds
      if cloud.x == cloud2.x and cloud.y == cloud2.y
        if (cloud.type == ct.water and cloud2.type == ct.acid) or (cloud.type == ct.water and cloud2.type == ct.acid)
          cloud2.alive = false
          cloud.alive = false
    if cloud.alive
      switch cloud.type
        when ct.water
          switch tile.type
            when tt.none
              tile.type = tt.water
            when tt.acid
              tile.type = tt.water
            when tt.grass
              tile.type = tt.plant
            when tt.plant
              tile.type = tt.tree
            when tt.tree
              tile.type = tt.forest
        when ct.acid
          switch tile.type
            when tt.water
              tile.type = tt.acid
            when tt.grass
              tile.type = tt.none
            when tt.plant
              tile.type = tt.grass
            when tt.tree
              tile.type = tt.plant
            when tt.forest
              tile.type = tt.tree


    updatetile(tile)

updateinfo = () ->
  $("#stats").html stats_temp(getstats())
  $("#storage").html res_temp(resources)

$ () ->
  #$("#sky").animate {top: -2000}, 5000
  $("#sky").hide()
  mapgen()
  getnewcursor()
  draw()
  updateinfo()

  $('.restart').live 'click', () ->
    if $(this).hasClass('locked')
      return
    $("#map").html('')
    mapgen($(this).data('robot'))
    draw()
    updateinfo()

  $('.res').live 'click', () ->
    newcc = $(this).data('robot')
    switch newcc
      when cursor.drill, cursor.grass
        switchcursor(newcc)
      when cursor.laser
        if resources.laser > 0 then switchcursor(newcc)
      when cursor.plant
        if resources.plant > 0 then switchcursor(newcc)
      when cursor.tree
        if resources.tree > 0 then switchcursor(newcc)
      when cursor.forest
        if resources.forest > 0 then switchcursor(newcc)
      when cursor.cloud
        if resources.cloud > 0 then switchcursor(newcc)

    return false

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
      when cursor.plant
        switch tile.type
          when tt.none
            tile.type = tt.plant
          else
            consumed = false
      when cursor.tree
        switch tile.type
          when tt.none
            tile.type = tt.tree
          else
            consumed = false
      when cursor.forest
        switch tile.type
          when tt.none
            tile.type = tt.forest
          else
            consumed = false
      when cursor.drill
        switch tile.type
          when tt.trash
            tile.type = tt.none
            resources.laser += 1
          when tt.grass
            tile.type = tt.none
          when tt.plant
            tile.type = tt.none
            resources.plant += 1
          when tt.tree
            tile.type = tt.none
            resources.tree += 1
          when tt.forest
            tile.type = tt.none
            resources.forest += 1
          else
            consumed = false
      when cursor.laser
        switch tile.type
          when tt.ice
            tile.type = tt.water
            resources.laser -= 1
          when tt.water
            tile.type = tt.none
            resources.laser -= 1
            resources.cloud += 1
          when tt.none
            tile.type = tt.none
            resources.laser -= 1
      when cursor.cloud
        if not addCloud(data.x, data.y, ct.water)
          consumed = false
        resources.cloud -= 1
      else
        consumed = false
        
    if consumed
      updatetile(tile)

      while true
        break unless checktile(tile)

      checkclouds()
      updateclouds()

      getnewcursor()

    updateinfo()

    return
    


