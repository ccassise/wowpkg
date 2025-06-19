namespace tests.Integration;

public class IntegrationTest1
{
    [Fact, Trait("Category", "Integration")]
    public void Test1()
    {
        Assert.Equal(1, 2);
    }
}