(function() {
  var addCloud, cc, checkclouds, checktile, cloud_temp, clouds, ct, cursor, cursors, draw, getnewcursor, getsim, getstats, getsurrounding, map, mapgen, mapsize, res_temp, resources, stats_temp, switchcursor, tile_temp, tt, tts, unlocked, updateclouds, updateinfo, updatetile;

  tile_temp = _.template('\
<div\
  class="tile tile_<%=data.x%>_<%=data.y%> tiletype_<%=data.type%>"\
  data-robot=\'<%=JSON.stringify(data)%>\'>\
</div>');

  cloud_temp = _.template('\
<div\
  class="cloud cloudid_<%=data.id%> cloudtype_<%=data.type%>"\
  style="top:<%=data.x*50%>px; left:<%=data.y*50%>px"\
  data-robot=\'<%=JSON.stringify(data)%>\'>\
</div>');

  res_temp = _.template('\
<b>Storage</b><br/>\
<a href="" class="res" data-robot="2">grass:</a> infin<br/>\
<a href="" class="res" data-robot="0">drill:</a> infin<br/>\
<a href="" class="res" data-robot="1">laser:</a>  <%=laser%><br/>\
<a href="" class="res" data-robot="3">plant:</a>  <%=plant%>  <br/>\
<a href="" class="res" data-robot="4">tree:</a>   <%=tree%>   <br/>\
<a href="" class="res" data-robot="5">forest:</a> <%=forest%> <br/>\
<a href="" class="res" data-robot="6">cloud:</a>  <%=cloud%>  <br/>\
');

  stats_temp = _.template('\
<b>Progres</b><br/>\
Water: <%=water%> <br/>\
Plants:  <%=plant%>  <br/>\
');

  mapsize = 10;

  cursor = {
    drill: 0,
    laser: 1,
    grass: 2,
    plant: 3,
    tree: 4,
    forest: 5,
    cloud: 6
  };

  tt = {
    none: 0,
    ice: 1,
    water: 2,
    trash: 3,
    acid: 4,
    grass: 100,
    plant: 101,
    tree: 102,
    forest: 103
  };

  ct = {
    none: -1,
    acid: 0,
    water: 1
  };

  resources = {
    laser: 0,
    plant: 0,
    tree: 0,
    forest: 0,
    cloud: 0
  };

  tts = _(tt).chain().values().map(function(x) {
    return 'tiletype_' + x;
  }).value().join(' ');

  cursors = _(cursor).chain().values().map(function(x) {
    return 'cursor_' + x;
  }).value().join(' ');

  cc = 0;

  map = [];

  clouds = [];

  unlocked = 0;

  mapgen = function(diff) {
    var cloudc, _results;
    if (diff == null) diff = 0;
    map = [];
    clouds = [];
    map.push(_(_.range(mapsize)).map(function(x) {
      return _(_.range(mapsize)).map(function(y) {
        var type;
        type = tt.none;
        switch (diff) {
          case 0:
            type = Math.floor(Math.random() * 4);
            break;
          case 1:
            type = Math.floor(Math.random() * 5);
            break;
          case 2:
            type = Math.floor(Math.random() * 5);
            if (type === tt.water) type = tt.trash;
            break;
          case 3:
            type = Math.floor(Math.random() * 5);
            if (type === tt.water) type = tt.trash;
            if (type === tt.water) type = tt.trash;
        }
        return {
          type: type,
          x: x,
          y: y
        };
      });
    }));
    cloudc = 0;
    switch (diff) {
      case 1:
        cloudc = 10;
        break;
      case 2:
        cloudc = 20;
        break;
      case 3:
        cloudc = 40;
    }
    _results = [];
    while (clouds.length < cloudc) {
      _results.push(addCloud(Math.floor(Math.random() * mapsize), Math.floor(Math.random() * mapsize), ct.acid));
    }
    return _results;
  };

  draw = function() {
    var x, y, _ref, _results;
    _results = [];
    for (x = 0, _ref = mapsize - 1; 0 <= _ref ? x <= _ref : x >= _ref; 0 <= _ref ? x++ : x--) {
      _results.push((function() {
        var _ref2, _results2;
        _results2 = [];
        for (y = 0, _ref2 = mapsize - 1; 0 <= _ref2 ? y <= _ref2 : y >= _ref2; 0 <= _ref2 ? y++ : y--) {
          _results2.push(map[0][x][y].tile = $(tile_temp({
            data: {
              x: x,
              y: y,
              type: map[0][x][y].type
            }
          })).appendTo("#map"));
        }
        return _results2;
      })());
    }
    return _results;
  };

  switchcursor = function(newcc) {
    cc = newcc;
    $("#cursor").removeClass(cursors).addClass('cursor_' + cc);
    return $("#info").removeClass().addClass('ct_' + cc);
  };

  getnewcursor = function() {
    cc = 2;
    $("#cursor").removeClass(cursors).addClass('cursor_' + cc);
    return $("#info").removeClass().addClass('ct_' + cc);
  };

  getsurrounding = function(x, y, type) {
    return _([[x + 1, y], [x - 1, y], [x, y + 1], [x, y - 1]]).chain().filter(function(x) {
      return x[0] >= 0 && x[0] < mapsize;
    }).filter(function(x) {
      return x[1] >= 0 && x[1] < mapsize;
    }).filter(function(x) {
      return type(map[0][x[0]][x[1]].type);
    }).value();
  };

  getsim = function(x, y, type) {
    var old_length, point, points, _i, _len;
    points = _.union([[x, y]], getsurrounding(x, y, type));
    old_length = 0;
    while (old_length !== points.length) {
      old_length = points.length;
      for (_i = 0, _len = points.length; _i < _len; _i++) {
        point = points[_i];
        points = _.union(points, getsurrounding(point[0], point[1], type));
        points = _.unique(points, false, function(x) {
          return x[0] + ',' + x[1];
        });
      }
    }
    return points;
  };

  updatetile = function(tile) {
    return tile.tile.removeClass(tts).addClass('tiletype_' + tile.type);
  };

  checktile = function(tile) {
    var blob, ti, type, _i, _len;
    switch (tile.type) {
      case tt.grass:
      case tt.plant:
      case tt.tree:
        blob = getsim(tile.x, tile.y, function(x) {
          return x === tile.type || x === tt.water;
        });
        break;
      default:
        return;
        blob = getsim(tile.x, tile.y, function(x) {
          return x === tile.type;
        });
    }
    switch (tile.type) {
      case tt.grass:
      case tt.plant:
      case tt.tree:
      case tt.water:
        if (blob.length >= 3) {
          type = tile.type;
          for (_i = 0, _len = blob.length; _i < _len; _i++) {
            ti = blob[_i];
            map[0][ti[0]][ti[1]].type = tt.none;
            updatetile(map[0][ti[0]][ti[1]]);
          }
          type = Math.min(tt.tree, type + Math.floor(blob.length / 3) - 1);
          switch (type) {
            case tt.tree:
              tile.type = tt.forest;
              break;
            case tt.plant:
              tile.type = tt.tree;
              break;
            case tt.grass:
              tile.type = tt.plant;
          }
          return updatetile(tile);
        }
    }
  };

  addCloud = function(x, y, type) {
    var cloud, _i, _len;
    for (_i = 0, _len = clouds.length; _i < _len; _i++) {
      cloud = clouds[_i];
      if (cloud.x === x && cloud.y === y) return false;
    }
    clouds.push({
      x: x,
      y: y,
      type: type
    });
    $("cloudid_" + clouds.length).show();
    return $(cloud_temp({
      data: {
        id: clouds.length,
        type: type === ct.acid ? 'acid' : 'water'
      }
    })).appendTo("#map");
  };

  getstats = function() {
    var cloud, plant, stats, water, wclouds, x, y, _i, _len, _ref, _ref2;
    stats = {};
    for (x = 0, _ref = mapsize - 1; 0 <= _ref ? x <= _ref : x >= _ref; 0 <= _ref ? x++ : x--) {
      for (y = 0, _ref2 = mapsize - 1; 0 <= _ref2 ? y <= _ref2 : y >= _ref2; 0 <= _ref2 ? y++ : y--) {
        stats[map[0][x][y].type] = stats[map[0][x][y].type] || 0;
        stats[map[0][x][y].type] += 1;
      }
    }
    wclouds = 0;
    for (_i = 0, _len = clouds.length; _i < _len; _i++) {
      cloud = clouds[_i];
      if (cloud.alive && cloud.type === ct.water) wclouds += 1;
    }
    plant = 0;
    plant += stats[tt.grass] || 0;
    plant += stats[tt.plant] * 2 || 0;
    plant += stats[tt.tree] * 4 || 0;
    plant += stats[tt.forest] * 8 || 0;
    water = stats[tt.water] + wclouds * 5;
    if (plant >= 100 && water >= 50) {
      unlocked += 1;
      $(".locked" + unlocked).removeClass("locked");
    }
    return {
      plant: (plant || 0) + '/100',
      water: (water || 0) + '/50'
    };
  };

  addCloud = function(x, y, type) {
    var cloud, id, _i, _len;
    for (_i = 0, _len = clouds.length; _i < _len; _i++) {
      cloud = clouds[_i];
      if (cloud.x === x && cloud.y === y) if (cloud.type === type) return false;
    }
    id = clouds.length;
    clouds.push({
      x: x,
      y: y,
      type: type,
      id: id,
      alive: true
    });
    return $(cloud_temp({
      data: {
        id: id,
        x: x,
        y: y,
        type: type === ct.acid ? 'acid' : 'water'
      }
    })).appendTo("#map");
  };

  updateclouds = function() {
    var cloud, newpos, x, y, _i, _len, _results;
    _results = [];
    for (_i = 0, _len = clouds.length; _i < _len; _i++) {
      cloud = clouds[_i];
      if (!cloud.alive) {
        $('.cloudid_' + cloud.id).hide();
        continue;
      }
      x = cloud.x;
      y = cloud.y;
      newpos = _([[x, y], [x + 1, y], [x - 1, y], [x, y + 1], [x, y - 1]]).chain().filter(function(x) {
        return x[0] >= 0 && x[0] < mapsize;
      }).filter(function(x) {
        return x[1] >= 0 && x[1] < mapsize;
      }).shuffle().value()[0];
      cloud.x = newpos[0];
      cloud.y = newpos[1];
      _results.push($('.cloudid_' + cloud.id).css({
        top: cloud.x * 50,
        left: cloud.y * 50
      }));
    }
    return _results;
  };

  checkclouds = function() {
    var cloud, cloud2, tile, _i, _j, _len, _len2, _results;
    _results = [];
    for (_i = 0, _len = clouds.length; _i < _len; _i++) {
      cloud = clouds[_i];
      tile = map[0][cloud.x][cloud.y];
      for (_j = 0, _len2 = clouds.length; _j < _len2; _j++) {
        cloud2 = clouds[_j];
        if (cloud.x === cloud2.x && cloud.y === cloud2.y) {
          if ((cloud.type === ct.water && cloud2.type === ct.acid) || (cloud.type === ct.water && cloud2.type === ct.acid)) {
            cloud2.alive = false;
            cloud.alive = false;
          }
        }
      }
      if (cloud.alive) {
        switch (cloud.type) {
          case ct.water:
            switch (tile.type) {
              case tt.none:
                tile.type = tt.water;
                break;
              case tt.acid:
                tile.type = tt.water;
                break;
              case tt.grass:
                tile.type = tt.plant;
                break;
              case tt.plant:
                tile.type = tt.tree;
                break;
              case tt.tree:
                tile.type = tt.forest;
            }
            break;
          case ct.acid:
            switch (tile.type) {
              case tt.water:
                tile.type = tt.acid;
                break;
              case tt.grass:
                tile.type = tt.none;
                break;
              case tt.plant:
                tile.type = tt.grass;
                break;
              case tt.tree:
                tile.type = tt.plant;
                break;
              case tt.forest:
                tile.type = tt.tree;
            }
        }
      }
      _results.push(updatetile(tile));
    }
    return _results;
  };

  updateinfo = function() {
    $("#stats").html(stats_temp(getstats()));
    return $("#storage").html(res_temp(resources));
  };

  $(function() {
    $("#sky").hide();
    mapgen();
    getnewcursor();
    draw();
    updateinfo();
    $('.restart').live('click', function() {
      if ($(this).hasClass('locked')) return;
      $("#map").html('');
      mapgen($(this).data('robot'));
      draw();
      return updateinfo();
    });
    $('.res').live('click', function() {
      var newcc;
      newcc = $(this).data('robot');
      switch (newcc) {
        case cursor.drill:
        case cursor.grass:
          switchcursor(newcc);
          break;
        case cursor.laser:
          if (resources.laser > 0) switchcursor(newcc);
          break;
        case cursor.plant:
          if (resources.plant > 0) switchcursor(newcc);
          break;
        case cursor.tree:
          if (resources.tree > 0) switchcursor(newcc);
          break;
        case cursor.forest:
          if (resources.forest > 0) switchcursor(newcc);
          break;
        case cursor.cloud:
          if (resources.cloud > 0) switchcursor(newcc);
      }
      return false;
    });
    return $('.tile').live('click', function() {
      var consumed, data, tile;
      data = $(this).data('robot');
      tile = map[0][data.x][data.y];
      consumed = true;
      switch (cc) {
        case cursor.grass:
          switch (tile.type) {
            case tt.none:
              tile.type = tt.grass;
              break;
            default:
              consumed = false;
          }
          break;
        case cursor.plant:
          switch (tile.type) {
            case tt.none:
              tile.type = tt.plant;
              break;
            default:
              consumed = false;
          }
          break;
        case cursor.tree:
          switch (tile.type) {
            case tt.none:
              tile.type = tt.tree;
              break;
            default:
              consumed = false;
          }
          break;
        case cursor.forest:
          switch (tile.type) {
            case tt.none:
              tile.type = tt.forest;
              break;
            default:
              consumed = false;
          }
          break;
        case cursor.drill:
          switch (tile.type) {
            case tt.trash:
              tile.type = tt.none;
              resources.laser += 1;
              break;
            case tt.grass:
              tile.type = tt.none;
              break;
            case tt.plant:
              tile.type = tt.none;
              resources.plant += 1;
              break;
            case tt.tree:
              tile.type = tt.none;
              resources.tree += 1;
              break;
            case tt.forest:
              tile.type = tt.none;
              resources.forest += 1;
              break;
            default:
              consumed = false;
          }
          break;
        case cursor.laser:
          switch (tile.type) {
            case tt.ice:
              tile.type = tt.water;
              resources.laser -= 1;
              break;
            case tt.water:
              tile.type = tt.none;
              resources.laser -= 1;
              resources.cloud += 1;
              break;
            case tt.none:
              tile.type = tt.none;
              resources.laser -= 1;
          }
          break;
        case cursor.cloud:
          if (!addCloud(data.x, data.y, ct.water)) consumed = false;
          resources.cloud -= 1;
          break;
        default:
          consumed = false;
      }
      if (consumed) {
        updatetile(tile);
        while (true) {
          if (!checktile(tile)) break;
        }
        checkclouds();
        updateclouds();
        getnewcursor();
      }
      updateinfo();
    });
  });

}).call(this);
