-- encoding: UTF-8

_CHINESE_DIGITS = {
  [0] = "〇",
  [1] = "一",
  [2] = "二",
  [3] = "三",
  [4] = "四",
  [5] = "五",
  [6] = "六",
  [7] = "七",
  [8] = "八",
  [9] = "九",
  [10] = "十",
}
_DATE_PATTERN = "^(%d+)-(%d+)-(%d+)$"
_TIME_PATTERN = "^(%d+):(%d+)$"

function get_chinese_math_num(num)
  local ret
  if num < 10 then
    ret = _CHINESE_DIGITS[num]
  elseif num < 20 then
    ret = _CHINESE_DIGITS[10]
    if num > 10 then
      ret = ret .. _CHINESE_DIGITS[num % 10]
    end
  elseif num < 100 then
    local mod = num % 10
    ret = _CHINESE_DIGITS[(num - mod) / 10] .. _CHINESE_DIGITS[10]
    if mod > 0 then
      ret = ret .. _CHINESE_DIGITS[mod]
    end
  else
    error("Invalid number")
  end
  return ret
end

function get_chinese_non_math_num(num)
  local ret = ""
  for ch in tostring(num):gmatch(".") do
    if ch >= "0" and ch <= "9" then
      ch = _CHINESE_DIGITS[tonumber(ch)]
    end
    ret = ret .. ch
  end
  return ret
end

function _verify_time(hour, minute)
  if hour < 0 or hour > 23 or minute < 0 or minute > 59 then
    error("Invalid time")
  end
end

function _verify_date(month, day)
  if month < 1 or month > 12 or day < 1 or day > _MONTH_TABLE_LEAF[month] then
    error("Invalid date")
  end
end

function _verify_date_with_year(year, month, day)
  _verify_date(month, day)
  if year < 1 or year > 9999 then
    error("Invalid year")
  end
  if month == 2 and day == 29 then
    if year % 400 ~= 0 and year % 100 == 0 then
      error("Invalid lunar day")
    end
    if year % 4 ~= 0 then
      error("Invalid lunar day")
    end
  end
end

function get_chinese_date(y, m, d, full)
  if full then
    return get_chinese_non_math_num(y) .. "年" ..
           get_chinese_math_num(m) .. "月" ..
           get_chinese_math_num(d) .. "日"
  else
    return y .. "年" .. m .. "月" .. d .. "日"
  end
end

function get_chinese_time(h, m, full)
  if full then
    local ret = get_chinese_math_num(h) .. "时"
    if m > 0 then
      ret = ret .. get_chinese_math_num(m) .. "分"
    end
    return ret
  else
    return h .. "时" .. m .. "分"
  end
end

function normalize_date(y, m, d)
  return string.format("%d-%02d-%02d", y, m, d)
end

function normalize_time(h, m)
  return string.format("%02d:%02d", h, m)
end

function get_time(input)
  local now = input
  if #input == 0 then
    now = os.date("%H:%M")
  end
  local hour, minute
  now:gsub(_TIME_PATTERN, function(h, m)
    hour = tonumber(h)
    minute = tonumber(m)
  end)
  _verify_time(hour, minute)
  return {
    normalize_time(hour, minute),
    get_chinese_time(hour, minute, false),
    get_chinese_time(hour, minute, true),
  }
end

function get_date(input)
  local now = input
  if #input == 0 then
    now = os.date("%Y-%m-%d")
  end
  local year, month, day
  now:gsub(_DATE_PATTERN, function(y, m, d)
    year = tonumber(y)
    month = tonumber(m)
    day = tonumber(d)
  end)
  _verify_date_with_year(year, month, day)
  return {
    normalize_date(year, month, day),
    get_chinese_date(year, month, day, false),
    get_chinese_date(year, month, day, true),
  }
end

----------------------------------

_MATH_KEYWORDS = {
  "abs", "acos", "asin", "atan", "atan2", "ceil", "cos", "cosh", "deg", "exp",
  "floor", "fmod", "frexp", "ldexp", "log", "log10", "max", "min", "modf", "pi",
  "pow", "rad", "random", "randomseed", "sin", "sinh", "sqrt", "tan", "tanh",
}

function _add_math_keyword(input)
  local ret = input
  for _, keyword in pairs(_MATH_KEYWORDS) do
    ret = ret:gsub(string.format("([^%%a\.])(%s\(.-\))", keyword), "%1math\.%2")
    ret = ret:gsub(string.format("^(%s\(.-\))", keyword), "math\.%1")
  end
  return ret
end

function compute(input)
  local expr = "return " .. _add_math_keyword(input)
  local func = loadstring(expr)
  if func == nil then
    return "-- 未完整表达式 --"
  end
  local ret = func()
  if ret == math.huge then -- div/0
    return "-- 计算错误 --"
  end
  if ret ~= ret then
    -- We rely on the property that NaN is the only value not equal to itself.
    return "-- 计算错误 --"
  end
  return ret
end

----------------------------------

_TO_BE_REPLACED_FLAG = "#TO_BE_REPLACED#"

_ASCII_IMAGE_TABLE = {

["birthday"] = { [[

..........................................................................

                H A P P Y  B I R T H D A Y  #TO_BE_REPLACED# !

..........................................................................
.....................**............................*......................
.....................++..............**..........*+.*.....................
...................*+*+..............**..........*++*.....................
.................*+***++*............*+........*.*.*++*...................
.................**..*+*.*..........*+.......*..*+.*.**...................
................+*.**++**+*........*+*+*.....*.*+**+***...................
................+**+*.*+**+*.....*+*.*++*.....**+**++*....................
.................**++**+*++*....**..*++.*+.....**+..+*....................
..................*+++.++*......+*.*+***.+.......*.+..**..................
.............********..*.***...*.**+*.*+**.....+*+..****..................
............*+*...****...+*....*+++++**++.....********....................
.............++....***+++**....+.*******....****...*******................
..............*+*........*+******+***..****.**+.......*+**................
..............*+*..........+++...***+.#+**...*+*.......+*.................
..............*+*..........+**+*.....**+++*..++.........++................
..............*+*..........**.*+*........*+*..*+*......*+**...............
..............*+*............+++*..........**.*+*........*++..............
..............*+*............+++*...........*+*..........*++..............
...........***+.*............+++*...........*+*..........*+***............
.......******.*+*............+++*...........*+*..........*+*.++*..........
......***......++...........*+*+*...........*++*************...**++.......
....*+*.**......***************+*...........*+*********....*.....***......
....++...*+**.................*+*...........*+*............*....**.**.....
...*+*.....*******.............*+*..........*+*....**********.***...**....
...*+*...*......************....*************...**..........****......*...
...++....*******.......***+++*********************************.......**...
..++.....++...*+*......*+***+*..........*********..........**...*..*..*...
..+*....*+*...*+......*+....*+*.......*+**.....*+*........**.....*+**++...
....*+.....**+.......**.....**........**........+*........**.......**+*...
...*.+......*+***.*****.....*+......***.........+*........+*.......*+**...
...*.+........******.........*********..........****..*****......***+*....
....*.*.....................................................**....*+......
....**+*...................................................+*****+*.......
........**++......................................*++*....*++*............
........*+******...............................*************..............
...............****++*###*******###########*****++........................
..........................................................................
]],
"生日蛋糕"},

["search"] = { [[

　　　　　　　　　　　--\--+--/--
　　　　　　　　　　　 {　o_o　}
┏━━━━━━━━━━oOo━(__)━oOo━┓
　#TO_BE_REPLACED#
┗━━━━━━━━━━━━━━━━━━┛
　　　┏━━━━┓　　┏━━━━┓
　　　┃　搜索　┃　　┃手气不错┃
　　　┗━━━━┛　　┗━━━━┛

]],
"搜索"},

}

function print_ascii(input)
  if #input <= 0 then
    local metatables = {}
    for k, v in pairs(_ASCII_IMAGE_TABLE) do
      table.insert(metatables, {["suggest"] = k, ["help"] = v[2]})
    end
    return metatables
  elseif _ASCII_IMAGE_TABLE[input] then
    local result  = _ASCII_IMAGE_TABLE[input][1]
    local last_commit = ime.get_last_commit()
    if #last_commit <= 0 or #last_commit > 20 then
      last_commit = ""
    end
    return result:gsub(_TO_BE_REPLACED_FLAG, last_commit)
  else
    error("Invalid argument")
  end
end

----------------------------------
-- Bitmaps table used for ascii_large_letters(), in 5x7 bitmaps.
_LARGE_LETTER_BITMAPS = {

[","] = [[




  ,,
  ,
 ,
]],

["-"] = [[



-----



]],

["."] = [[





..
..
]],

["0"] = [[
 000
0   0
0   0
0   0
0   0
0   0
 000
]],

["1"] = [[
  1
111
  1
  1
  1
  1
11111
]],

["2"] = [[
 222
2   2
    2
   2
  2
 2
22222
]],

["3"] = [[
 333
3   3
    3
  33
    3
3   3
 333
]],

["4"] = [[
   4
  44
 4 4
4  4
44444
   4
  444
]],

["5"] = [[
55555
5
5555
    5
    5
5   5
 555
]],

["6"] = [[
  66
 6
6
6666
6   6
6   6
 666
]],

["7"] = [[
77777
7   7
   7
  7
 7
 7
 7
]],

["8"] = [[
 888
8   8
8   8
 888
8   8
8   8
 888
]],

["9"] = [[
 999
9   9
9   9
 9999
    9
   9
 99
]],

["A"] = [[
  A
  A
 A A
 A A
AAAAA
A   A
A   A
]],

["B"] = [[
BBBB
B   B
B   B
BBBB
B   B
B   B
BBBB
]],

["C"] = [[
 CCC
C   C
C
C
C
C   C
 CCC
]],

["D"] = [[
DDD
D  D
D   D
D   D
D   D
D  D
DDD
]],

["E"] = [[
EEEEE
E
E
EEEE
E
E
EEEEE
]],

["F"] = [[
FFFFF
F
F
FFFF
F
F
F
]],

["G"] = [[
 GGG
G   G
G
G  GG
G   G
G   G
 GGG
]],

["H"] = [[
H   H
H   H
H   H
HHHHH
H   H
H   H
H   H
]],

["I"] = [[
IIIII
  I
  I
  I
  I
  I
IIIII
]],

["J"] = [[
JJJJJ
  J
  J
  J
  J
  J
JJ
]],

["K"] = [[
K   K
K  K
K K
KK
K K
K  K
K   K
]],

["L"] = [[
L
L
L
L
L
L
LLLLL
]],

["M"] = [[
 M M
 MMM
 MMM
 MMM
M M M
M M M
M   M
]],

["N"] = [[
N   N
N   N
NN  N
N N N
N  NN
N   N
N   N
]],

["O"] = [[
 OOO
O   O
O   O
O   O
O   O
O   O
 OOO
]],

["P"] = [[
PPPP
P   P
P   P
PPPP
P
P
P
]],

["Q"] = [[
 QQQ
Q   Q
Q   Q
Q   Q
Q Q Q
Q Q Q
 QQQQ
]],

["R"] = [[
RRRR
R   R
R   R
RRRR
R R
R  R
R   R
]],

["S"] = [[
 SSS
S   S
S
 SSS
    S
S   S
 SSS
]],

["T"] = [[
TTTTT
  T
  T
  T
  T
  T
  T
]],

["U"] = [[
U   U
U   U
U   U
U   U
U   U
U   U
 UUU
]],

["V"] = [[
V   V
V   V
V   V
 V V
 V V
 V V
  V
]],

["W"] = [[
W   W
W W W
W W W
 WWW
 WWW
 WWW
 W W
]],

["X"] = [[
X   X
X   X
 X X
  X
 X X
X   X
X   X
]],

["Y"] = [[
Y   Y
Y   Y
 Y Y
  Y
  Y
  Y
  Y
]],

["Z"] = [[
ZZZZZ
    Z
   Z
  Z
 Z
Z
ZZZZZ
]],

}

-- Converts input string to ascii image of large letters.
function print_letter(input_string)
  if #input_string == 0 then
    return {}
  end
  local letter_width = 5
  local letter_height = 7
  local max_string_length = 16
  local result_lines = {}
  for i = 1, letter_height do result_lines[i] = "" end
  input_string = input_string:upper()
  -- Limits the input string size.
  input_string = input_string:sub(1, max_string_length)
  -- Only interate on valid characters.
  for c in input_string:gmatch("[0-9A-Z ,.-]") do
    local letter = _LARGE_LETTER_BITMAPS[c]
    -- Splits the letter bitmap into lines and appends each line to the
    -- corresponding line of the result.
    local lines = ime.split_string(letter, "\n")
    for i, line in ipairs(lines) do
      if i > letter_height then break end
      for i = 1, letter_width - #line do line = line .. ' ' end
      result_lines[i] = result_lines[i] .. line .. '  '
    end
  end
  -- Merges result lines.
  for i, line in ipairs(result_lines) do
    result_lines[i] = ime.trim_string_right(result_lines[i])
  end
  local result = "\n" .. ime.join_string(result_lines, "\n") .. "\n"
  return result
end

-- Dice images.
_DICE_BITMAPS = {

[[
/---------\
|         |
|         |
|   (O)   |
|         |
|         |
\---------/
]],

[[
/---------\
|         |
|    @    |
|         |
|    @    |
|         |
\---------/
]],

[[
/---------\
|         |
|  @      |
|    @    |
|      @  |
|         |
\---------/
]],

[[
/---------\
|         |
|  @   @  |
|         |
|  @   @  |
|         |
\---------/
]],

[[
/---------\
|         |
|  @   @  |
|    @    |
|  @   @  |
|         |
\---------/
]],

[[
/---------\
|         |
|  @   @  |
|  @   @  |
|  @   @  |
|         |
\---------/
]],

}

math.randomseed(os.time())

-- Plays and shows n dices.
function play_dice(n)
  n = math.min(n, 6)
  n = math.max(n, 1)
  local dice_height = 7
  local result_lines = {}
  for i = 1, dice_height do result_lines[i] = "" end
  for i = 1, n do
    local index = math.random(1, 6)
    local dice = _DICE_BITMAPS[index]
    -- Splits the dice bitmap into lines and appends each line to the
    -- corresponding line of the result.
    local lines = ime.split_string(dice, "\n")
    for i, line in ipairs(lines) do
      if i > dice_height then break end
      result_lines[i] = result_lines[i] .. line .. '   '
    end
  end
  -- Merges result lines.
  for i, line in ipairs(result_lines) do
    result_lines[i] = ime.trim_string_right(result_lines[i])
  end
  local result = "\n" .. ime.join_string(result_lines, "\n") .. "\n"
  return result
end

--------------------------
_ZODIAC_TABLE = {
  [{3, 21, 4, 19}] = "白羊座(Aries) ♈",
  [{4, 20, 5, 20}] = "金牛座(Taurus) ♉",
  [{5, 21, 6, 21}] = "双子座(Gemini) ♊",
  [{6, 22, 7, 22}] = "巨蟹座(Cancer) ♋",
  [{7, 23, 8, 22}] = "狮子座(Leo) ♌",
  [{8, 23, 9, 23}] = "处女座(Virgo) ♍",
  [{9, 24, 10, 23}] = "天秤座(Libra) ♎",
  [{10, 24, 11, 21}] = "天蝎座(Scorpio) ♏",
  [{11, 22, 12, 21}] = "射手座(Sagittarius) ♐",
  [{12, 22, 12, 31}] = "摩羯座(Capricorn) ♑",
  [{1, 1, 1, 19}] = "摩羯座(Capricorn) ♑",
  [{1, 20, 2, 18}] = "水瓶座(Aquarius) ♒",
  [{2, 19, 3, 20}] = "双鱼座(Pisces) ♓",
}

_MONTH_TABLE_NORMAL = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 }
_MONTH_TABLE_LEAF = { 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 }

function _compute_month_and_day(month1, day1, month2, day2)
  if month1 < month2 then
    return -1
  elseif month1 > month2 then
    return 1
  elseif day1 < day2 then
    return -1
  elseif day1 > day2 then
    return 1
  else
    return 0
  end
end

-- birthday is a string in MM-DD format.
function query_zodiac(birthday)
  local month = 0
  local day = 0
  birthday:gsub("([0-9]+)-([0-9]+)$",
                function(m, d)
                  month = tonumber(m)
                  day = tonumber(d)
                end
               )
  _verify_date(month, day)
  for range, name in pairs(_ZODIAC_TABLE) do
    local from_month = range[1]
    local from_day = range[2]
    local to_month = range[3]
    local to_day = range[4]
    if _compute_month_and_day(month, day, from_month, from_day) >=0 and
       _compute_month_and_day(month, day, to_month, to_day) <=0 then
      return name
    end
  end
  error("Should never reach here")
end


-----

-- Print Chinese manuscript grids. width_and_height in "WxH" format.
-- ┏━━━━━━━━━┓
-- ┣━┳━┳━┳━┳━┫
-- ┃　┃　┃　┃　┃　┃
-- ┣━┻━┻━┻━┻━┫
-- ┣━┳━┳━┳━┳━┫
-- ┃　┃　┃　┃　┃　┃
-- ┣━┻━┻━┻━┻━┫
-- ┣━┳━┳━┳━┳━┫
-- ┃　┃　┃　┃　┃　┃
-- ┣━┻━┻━┻━┻━┫
-- ┗━━━━━━━━━┛
function print_gaozhi(width_and_height)
  local width
  local height
  width_and_height:gsub("([0-9]+)[x%*]([0-9]+)$",
                        function(w, h)
                          width = w
                          height = h
                        end
                       )
  width = math.min(width, 20)
  width = math.max(width, 1)
  height = math.min(height, 20)
  height = math.max(height, 1)
  local result = "\n"
  local print_line = function(leading, middle, middle_repeat, ending)
                       result = result .. leading
                       for i = 1, middle_repeat do
                         result = result .. middle
                       end
                       result = result .. ending
                       result = result .. "\n"
                     end
  print_line("┏", "━", width * 2 - 1, "┓")
  for i = 1, height do
    print_line("┣", "━┳", width - 1, "━┫")
    print_line("┃", "　┃", width, "")
    print_line("┣", "━┻", width - 1, "━┫")
  end
  print_line("┗", "━", width * 2 - 1, "┛")
  return result .. "\n"
end


------------
ime.register_command("sj", "get_time", "输入时间", "alpha", "输入可选时间，例如12:34")
ime.register_command("rq", "get_date", "输入日期", "alpha", "输入可选日期，例如2008-08-08")
ime.register_command("js", "compute", "计算模式", "none", "输入表达式，例如3*log(4+2)")
ime.register_command("gz", "print_gaozhi", "打印稿纸", "none", "输入稿纸大小，例如2x3")
ime.register_command("xz", "query_zodiac", "查询星座", "none", "输入您的生日，例如12-14")
ime.register_command("sz", "play_dice", "掷骰子", "none", "输入骰子个数，例如3")
ime.register_command("zf", "print_letter", "打印字符", "none", "请输入字母或数字序列，例如hello")
ime.register_command("hh", "print_ascii", "画画")

print("lua script loaded.")
