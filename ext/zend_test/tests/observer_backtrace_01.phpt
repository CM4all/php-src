--TEST--
Observer: Show backtrace on init
--EXTENSIONS--
zend_test
--INI--
zend_test.observer.enabled=1
zend_test.observer.show_output=1
zend_test.observer.observe_all=1
zend_test.observer.show_init_backtrace=1
--FILE--
<?php
class TestClass
{
    private function bar($number)
    {
        return $number + 2;
    }

    public function foo()
    {
        return array_map(function ($value) {
            return $this->bar($value);
        }, [40, 1335]);
    }
}

function gen()
{
    $test = new TestClass();
    yield $test->foo();
}

function foo()
{
    return gen()->current();
}

var_dump(foo());
?>
--EXPECTF--
<!-- init '%s%eobserver_backtrace_%d.php' -->
<!--
    {main} %s%eobserver_backtrace_%d.php
-->
<file '%s%eobserver_backtrace_%d.php'>
  <!-- init foo() -->
  <!--
      foo()
      {main} %s%eobserver_backtrace_%d.php
  -->
  <foo>
    <!-- init Generator::current() -->
    <!--
        Generator::current()
        foo()
        {main} %s%eobserver_backtrace_%d.php
    -->
    <Generator::current>
      <!-- init gen() -->
      <!--
          gen()
          Generator::current()
          foo()
          {main} %s%eobserver_backtrace_%d.php
      -->
      <gen>
        <!-- init TestClass::foo() -->
        <!--
            TestClass::foo()
            gen()
            Generator::current()
            foo()
            {main} %s%eobserver_backtrace_%d.php
        -->
        <TestClass::foo>
          <!-- init array_map() -->
          <!--
              array_map()
              TestClass::foo()
              gen()
              Generator::current()
              foo()
              {main} %s%eobserver_backtrace_%d.php
          -->
          <array_map>
            <!-- init TestClass::{closure}() -->
            <!--
                TestClass::{closure}()
                array_map()
                TestClass::foo()
                gen()
                Generator::current()
                foo()
                {main} %s%eobserver_backtrace_%d.php
            -->
            <TestClass::{closure}>
              <!-- init TestClass::bar() -->
              <!--
                  TestClass::bar()
                  TestClass::{closure}()
                  array_map()
                  TestClass::foo()
                  gen()
                  Generator::current()
                  foo()
                  {main} %s%eobserver_backtrace_%d.php
              -->
              <TestClass::bar>
              </TestClass::bar>
            </TestClass::{closure}>
            <TestClass::{closure}>
              <TestClass::bar>
              </TestClass::bar>
            </TestClass::{closure}>
          </array_map>
        </TestClass::foo>
      </gen>
    </Generator::current>
  </foo>
  <!-- init var_dump() -->
  <!--
      var_dump()
      {main} %s%eobserver_backtrace_%d.php
  -->
  <var_dump>
array(2) {
  [0]=>
  int(42)
  [1]=>
  int(1337)
}
  </var_dump>
</file '%s%eobserver_backtrace_%d.php'>
