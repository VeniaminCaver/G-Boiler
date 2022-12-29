## Как внести изменения в [G-Boiler](https://github.com/VeniaminCaver/G-Boiler) или как помочь проекту (как стать "contributer")?

1. Форкаем оригинальный проект https://github.com/VeniaminCaver/G-Boiler.git кнопкой "Fork".

2. Клонируем СВОЙ форк к СЕБЕ на компьютер:
```
git clone git@github.com:YOURGITHUBACCOUNT/G-Boiler.git
cd G-Boiler
```

3. Создаем новую ветку:
```
git checkout -b myfix
```

4. Создаем upstream на оригинальный проект:
```
git remote add upstream git@github.com:VeniaminCaver/G-Boiler.git
```
5. Меняем файлы
6. Делаем коммит и отправляем правки

```
git add .
git commit -am "Мои улучшения"
git push -u origin myfix
```
7. Переходим в свой проект https://github.com/YOURGITHUBACCOUNT/G-Boiler и жмем кнопку Compare & pull
8. Описываем какую проблему решает Пул Реквест с кратким описанием, зачем сделано изменение
9. Вы прекрасны! ;)
